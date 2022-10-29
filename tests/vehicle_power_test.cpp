#include <cstdlib>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "map.h"
#include "map_helpers.h"
#include "options_helpers.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "weather.h"
#include "weather_type.h"

static const efftype_id effect_blind( "blind" );

static const itype_id fuel_type_battery( "battery" );
static const itype_id fuel_type_plut_cell( "plut_cell" );

static const vproto_id vehicle_prototype_reactor_test( "reactor_test" );
static const vproto_id vehicle_prototype_scooter_electric_test( "scooter_electric_test" );
static const vproto_id vehicle_prototype_scooter_test( "scooter_test" );
static const vproto_id vehicle_prototype_solar_panel_test( "solar_panel_test" );


// TODO: Move this into player_helpers to avoid character include.
static void reset_player()
{
    Character &player_character = get_player_character();
    // Move player somewhere safe
    REQUIRE( !player_character.in_vehicle );
    player_character.setpos( tripoint_zero );
    // Blind the player to avoid needless drawing-related overhead
    player_character.add_effect( effect_blind, 1_turns, true );
}

TEST_CASE( "vehicle power with reactor", "[vehicle][power]" )
{
    clear_vehicles();
    reset_player();
    build_test_map( ter_id( "t_pavement" ) );
    map &here = get_map();

    SECTION( "vehicle with reactor" ) {
        const tripoint reactor_origin = tripoint( 10, 10, 0 );
        vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_reactor_test, reactor_origin,
                                             0_degrees, 0, 0 );
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
}

TEST_CASE( "Solar power", "[vehicle][power]" )
{
    clear_vehicles();
    reset_player();
    build_test_map( ter_id( "t_pavement" ) );
    map &here = get_map();

    const tripoint solar_origin = tripoint( 5, 5, 0 );
    vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_solar_panel_test, solar_origin, 0_degrees, 0,
                                         0 );
    REQUIRE( veh_ptr != nullptr );
    REQUIRE_FALSE( veh_ptr->solar_panels.empty() );
    scoped_weather_override clear_weather( WEATHER_CLEAR );

    SECTION( "Summer day noon" ) {
        calendar::turn = calendar::turn_zero + calendar::season_length() + 1_days + 12_hours;
        veh_ptr->update_time( calendar::turn );
        veh_ptr->discharge_battery( 100000 );
        REQUIRE( veh_ptr->fuel_left( fuel_type_battery ) == 0 );
        WHEN( "30 minutes elapse" ) {
            veh_ptr->update_time( calendar::turn + 30_minutes );
            int power = veh_ptr->fuel_left( fuel_type_battery );
            CHECK( power == Approx( 425 ).margin( 1 ) );
        }
    }

    SECTION( "Summer before sunrise" ) {
        calendar::turn = calendar::turn_zero + calendar::season_length() + 1_days;
        calendar::turn = sunrise( calendar::turn ) - 1_hours;
        veh_ptr->update_time( calendar::turn );
        veh_ptr->discharge_battery( 100000 );
        REQUIRE( veh_ptr->fuel_left( fuel_type_battery ) == 0 );
        WHEN( "30 minutes elapse" ) {
            veh_ptr->update_time( calendar::turn + 30_minutes );
            int power = veh_ptr->fuel_left( fuel_type_battery );
            CHECK( power == 0 );
        }
    }

    SECTION( "Winter noon" ) {
        calendar::turn = calendar::turn_zero + 3 * calendar::season_length() + 1_days + 12_hours;
        veh_ptr->update_time( calendar::turn );
        veh_ptr->discharge_battery( 100000 );
        REQUIRE( veh_ptr->fuel_left( fuel_type_battery ) == 0 );
        WHEN( "30 minutes elapse" ) {
            veh_ptr->update_time( calendar::turn + 30_minutes );
            int power = veh_ptr->fuel_left( fuel_type_battery );
            CHECK( power == Approx( 182 ).margin( 1 ) );
        }
    }

    SECTION( "2x 30 minutes produces same power as 1x 60 minutes" ) {
        const tripoint solar_origin_2 = tripoint( 10, 10, 0 );
        vehicle *veh_2_ptr = here.add_vehicle( vehicle_prototype_solar_panel_test, solar_origin_2,
                                               0_degrees, 0,
                                               0 );
        REQUIRE( veh_ptr != nullptr );
        REQUIRE( veh_2_ptr != nullptr );


        calendar::turn = calendar::turn_zero + 1_days;
        veh_ptr->update_time( calendar::turn );
        veh_2_ptr->update_time( calendar::turn );

        veh_ptr->discharge_battery( 100000 );
        veh_2_ptr->discharge_battery( 100000 );
        REQUIRE( veh_ptr->fuel_left( fuel_type_battery ) == 0 );
        REQUIRE( veh_2_ptr->fuel_left( fuel_type_battery ) == 0 );

        // Vehicle 1 does 2x 30 minutes while vehicle 2 does 1x 60 minutes
        veh_ptr->update_time( calendar::turn + 30_minutes );
        veh_ptr->update_time( calendar::turn + 60_minutes );
        veh_2_ptr->update_time( calendar::turn + 60_minutes );

        int power = veh_ptr->fuel_left( fuel_type_battery );
        int power_2 = veh_2_ptr->fuel_left( fuel_type_battery );
        CHECK( power == Approx( power_2 ).margin( 1 ) );
    }
}

TEST_CASE( "Daily solar power", "[vehicle][power]" )
{
    clear_vehicles();
    reset_player();
    build_test_map( ter_id( "t_pavement" ) );
    map &here = get_map();

    const tripoint solar_origin = tripoint( 5, 5, 0 );
    vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_solar_panel_test, solar_origin, 0_degrees, 0,
                                         0 );
    REQUIRE( veh_ptr != nullptr );
    scoped_weather_override clear_weather( WEATHER_CLEAR );

    SECTION( "Spring day 2" ) {
        calendar::turn = calendar::turn_zero + 1_days;
        veh_ptr->update_time( calendar::turn );
        veh_ptr->discharge_battery( 100000 );
        REQUIRE( veh_ptr->fuel_left( fuel_type_battery ) == 0 );
        WHEN( "24 hours pass" ) {
            veh_ptr->update_time( calendar::turn + 24_hours );
            int power = veh_ptr->fuel_left( fuel_type_battery );
            CHECK( power == Approx( 5184 ).margin( 1 ) );
        }
    }

    SECTION( "Summer day 1" ) {
        calendar::turn = calendar::turn_zero + calendar::season_length();
        veh_ptr->update_time( calendar::turn );
        veh_ptr->discharge_battery( 100000 );
        REQUIRE( veh_ptr->fuel_left( fuel_type_battery ) == 0 );
        WHEN( "24 hours pass" ) {
            veh_ptr->update_time( calendar::turn + 24_hours );
            int power = veh_ptr->fuel_left( fuel_type_battery );
            CHECK( power == Approx( 7863 ).margin( 1 ) );
        }
    }

    SECTION( "Autum day 1" ) {
        calendar::turn = calendar::turn_zero + 2 * calendar::season_length();
        veh_ptr->update_time( calendar::turn );
        veh_ptr->discharge_battery( 100000 );
        REQUIRE( veh_ptr->fuel_left( fuel_type_battery ) == 0 );
        WHEN( "24 hours pass" ) {
            veh_ptr->update_time( calendar::turn + 24_hours );
            int power = veh_ptr->fuel_left( fuel_type_battery );
            CHECK( power == Approx( 5098 ).margin( 1 ) );
        }
    }

    SECTION( "Winter day 1" ) {
        calendar::turn = calendar::turn_zero + 3 * calendar::season_length();
        veh_ptr->update_time( calendar::turn );
        veh_ptr->discharge_battery( 100000 );
        REQUIRE( veh_ptr->fuel_left( fuel_type_battery ) == 0 );
        WHEN( "24 hours pass" ) {
            veh_ptr->update_time( calendar::turn + 24_hours );
            int power = veh_ptr->fuel_left( fuel_type_battery );
            CHECK( power == Approx( 2074 ).margin( 1 ) );
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
        vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_scooter_test, origin, 0_degrees, 0, 0 );
        REQUIRE( veh_ptr != nullptr );
        veh_ptr->charge_battery( 450 );
        REQUIRE( veh_ptr->fuel_left( fuel_type_battery ) == 450 );

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
        vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_scooter_electric_test, origin,
                                             0_degrees, 0, 0 );
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

