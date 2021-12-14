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

static const vproto_id vehicle_prototype_scooter_electric_test( "scooter_electric_test" );
static const vproto_id vehicle_prototype_scooter_test( "scooter_test" );
static const vproto_id vehicle_prototype_solar_panel_test( "solar_panel_test" );

static const weather_type_id weather_sunny( "sunny" );

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

