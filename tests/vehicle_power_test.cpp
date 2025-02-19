#include <cstdlib>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "debug.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "options_helpers.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather_type.h"

static const efftype_id effect_blind( "blind" );

static const itype_id fuel_type_battery( "battery" );
static const itype_id itype_test_power_cord_25_loss( "test_power_cord_25_loss" );

static const vpart_id vpart_frame( "frame" );
static const vpart_id vpart_small_storage_battery( "small_storage_battery" );

static const vproto_id vehicle_prototype_none( "none" );
static const vproto_id vehicle_prototype_scooter_electric_test( "scooter_electric_test" );
static const vproto_id vehicle_prototype_scooter_test( "scooter_test" );
static const vproto_id vehicle_prototype_solar_panel_test( "solar_panel_test" );

// TODO: Move this into player_helpers to avoid character include.
static void reset_player()
{
    map &here = get_map();

    Character &player_character = get_player_character();
    // Move player somewhere safe
    REQUIRE( !player_character.in_vehicle );
    player_character.setpos( here, tripoint_bub_ms::zero );
    // Blind the player to avoid needless drawing-related overhead
    player_character.add_effect( effect_blind, 1_turns, true );
}

TEST_CASE( "power_loss_to_cables", "[vehicle][power]" )
{
    clear_vehicles();
    reset_player();
    build_test_map( ter_id( "t_pavement" ) );
    map &here = get_map();

    const auto connect_debug_cord = [&here]( const tripoint_bub_ms & source,
    const tripoint_bub_ms & target ) {
        const optional_vpart_position target_vp = here.veh_at( target );
        const optional_vpart_position source_vp = here.veh_at( source );

        item cord( itype_test_power_cord_25_loss );
        cord.set_var( "source_x", source.x() );
        cord.set_var( "source_y", source.y() );
        cord.set_var( "source_z", source.z() );
        cord.set_var( "state", "pay_out_cable" );
        cord.active = true;

        if( !target_vp ) {
            debugmsg( "missing target at %s", target.to_string() );
        }
        vehicle *const target_veh = &target_vp->vehicle();
        vehicle *const source_veh = &source_vp->vehicle();
        if( source_veh == target_veh ) {
            debugmsg( "source same as target" );
        }

        tripoint_abs_ms target_global = here.get_abs( target );
        const vpart_id vpid( cord.typeId().str() );

        point_rel_ms vcoords = source_vp->mount_pos();
        vehicle_part source_part( vpid, item( cord ) );
        source_part.target.first = target_global;
        source_part.target.second = target_veh->pos_abs();
        source_veh->install_part( here, vcoords, std::move( source_part ) );

        vcoords = target_vp->mount_pos();
        vehicle_part target_part( vpid, item( cord ) );
        tripoint_bub_ms source_global( cord.get_var( "source_x", 0 ),
                                       cord.get_var( "source_y", 0 ),
                                       cord.get_var( "source_z", 0 ) );
        target_part.target.first = here.get_abs( source_global );
        target_part.target.second = source_veh->pos_abs();
        target_veh->install_part( here, vcoords, std::move( target_part ) );
    };

    const std::vector<tripoint_bub_ms> placements { { 4, 10, 0 }, { 6, 10, 0 }, { 8, 10, 0 } };
    std::vector<vpart_reference> batteries;
    for( const tripoint_bub_ms &p : placements ) {
        REQUIRE( !here.veh_at( p ).has_value() );
        vehicle *veh = here.add_vehicle( vehicle_prototype_none, p, 0_degrees, 0, 0 );
        REQUIRE( veh != nullptr );
        const int frame_part_idx = veh->install_part( here, point_rel_ms::zero, vpart_frame );
        REQUIRE( frame_part_idx != -1 );
        const int bat_part_idx = veh->install_part( here, point_rel_ms::zero, vpart_small_storage_battery );
        REQUIRE( bat_part_idx != -1 );
        veh->refresh( );
        here.add_vehicle_to_cache( veh );
        batteries.emplace_back( *veh, bat_part_idx );
    }
    // connect first to second and second to third, each cord is 25% lossy
    // third battery will on average take twice as many charges to charge as the first
    for( size_t i = 0; i < placements.size() - 1; i++ ) {
        connect_debug_cord( placements[i], placements[i + 1] );
    }
    const optional_vpart_position ovp_first = here.veh_at( placements[0] );
    REQUIRE( ovp_first.has_value() );
    vehicle &v = ovp_first->vehicle(); // charge first battery
    struct preset_t {
        const int charge;                // how much charge to expect
        const int max_charge_excess;     // minimum expect to spill out
        const int max_charge_in_battery; // max charge expected in each battery
        const int discharge;             // how much to discharge
        const int min_discharge_deficit; // minimum deficit after discharge
    };
    const std::vector<preset_t> presets {
        { 1000,    0,  250, 3000, 2000 },
        { 3000,    0,  750, 5000, 3000 },
        { 9000, 5500, 1000, 9000, 6000 },
    };
    for( const preset_t &preset : presets ) {
        REQUIRE( v.fuel_left( here, fuel_type_battery ) == 0 ); // ensure empty batteries
        const int remainder = v.charge_battery( here, preset.charge );
        CHECK( remainder <= preset.max_charge_excess );
        for( size_t i = 0; i < batteries.size(); i++ ) {
            CAPTURE( i );
            CHECK( preset.max_charge_in_battery >= batteries[i].part().ammo_remaining( ) );
        }
        const int deficit = v.discharge_battery( here, preset.discharge );
        CHECK( deficit >= preset.min_discharge_deficit );
    }
}

TEST_CASE( "Solar_power", "[vehicle][power]" )
{
    clear_vehicles();
    reset_player();
    build_test_map( ter_id( "t_pavement" ) );
    map &here = get_map();

    const tripoint_bub_ms solar_origin{ 5, 5, 0 };
    vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_solar_panel_test, solar_origin, 0_degrees, 0,
                                         0 );
    REQUIRE( veh_ptr != nullptr );
    REQUIRE_FALSE( veh_ptr->solar_panels.empty() );
    scoped_weather_override clear_weather( WEATHER_CLEAR );

    SECTION( "Summer day noon" ) {
        calendar::turn = calendar::turn_zero + calendar::season_length() + 1_days + 12_hours;
        veh_ptr->update_time( here,  calendar::turn );
        veh_ptr->discharge_battery( here, 100000 );
        REQUIRE( veh_ptr->fuel_left( here, fuel_type_battery ) == 0 );
        WHEN( "30 minutes elapse" ) {
            veh_ptr->update_time( here, calendar::turn + 30_minutes );
            int power = veh_ptr->fuel_left( here, fuel_type_battery );
            CHECK( power == Approx( 425 ).margin( 1 ) );
        }
    }

    SECTION( "Summer before sunrise" ) {
        calendar::turn = calendar::turn_zero + calendar::season_length() + 1_days;
        calendar::turn = sunrise( calendar::turn ) - 1_hours;
        veh_ptr->update_time( here, calendar::turn );
        veh_ptr->discharge_battery( here, 100000 );
        REQUIRE( veh_ptr->fuel_left( here, fuel_type_battery ) == 0 );
        WHEN( "30 minutes elapse" ) {
            veh_ptr->update_time( here, calendar::turn + 30_minutes );
            int power = veh_ptr->fuel_left( here, fuel_type_battery );
            CHECK( power == 0 );
        }
    }

    SECTION( "Winter noon" ) {
        calendar::turn = calendar::turn_zero + 3 * calendar::season_length() + 1_days + 12_hours;
        veh_ptr->update_time( here, calendar::turn );
        veh_ptr->discharge_battery( here, 100000 );
        REQUIRE( veh_ptr->fuel_left( here, fuel_type_battery ) == 0 );
        WHEN( "30 minutes elapse" ) {
            veh_ptr->update_time( here, calendar::turn + 30_minutes );
            int power = veh_ptr->fuel_left( here, fuel_type_battery );
            CHECK( power == Approx( 184 ).margin( 1 ) );
        }
    }

    SECTION( "2x 30 minutes produces same power as 1x 60 minutes" ) {
        const tripoint_bub_ms solar_origin_2{ 10, 10, 0 };
        vehicle *veh_2_ptr = here.add_vehicle( vehicle_prototype_solar_panel_test, solar_origin_2,
                                               0_degrees, 0,
                                               0 );
        REQUIRE( veh_ptr != nullptr );
        REQUIRE( veh_2_ptr != nullptr );

        calendar::turn = calendar::turn_zero + 1_days;
        veh_ptr->update_time( here, calendar::turn );
        veh_2_ptr->update_time( here, calendar::turn );

        veh_ptr->discharge_battery( here, 100000 );
        veh_2_ptr->discharge_battery( here, 100000 );
        REQUIRE( veh_ptr->fuel_left( here, fuel_type_battery ) == 0 );
        REQUIRE( veh_2_ptr->fuel_left( here, fuel_type_battery ) == 0 );

        // Vehicle 1 does 2x 30 minutes while vehicle 2 does 1x 60 minutes
        veh_ptr->update_time( here, calendar::turn + 30_minutes );
        veh_ptr->update_time( here, calendar::turn + 60_minutes );
        veh_2_ptr->update_time( here, calendar::turn + 60_minutes );

        int power = veh_ptr->fuel_left( here, fuel_type_battery );
        int power_2 = veh_2_ptr->fuel_left( here, fuel_type_battery );
        CHECK( power == Approx( power_2 ).margin( 1 ) );
    }
}

TEST_CASE( "Daily_solar_power", "[vehicle][power]" )
{
    clear_vehicles();
    reset_player();
    build_test_map( ter_id( "t_pavement" ) );
    map &here = get_map();

    const tripoint_bub_ms solar_origin{ 5, 5, 0 };
    vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_solar_panel_test, solar_origin, 0_degrees, 0,
                                         0 );
    REQUIRE( veh_ptr != nullptr );
    scoped_weather_override clear_weather( WEATHER_CLEAR );

    SECTION( "Spring day 2" ) {
        calendar::turn = calendar::turn_zero + 1_days;
        veh_ptr->update_time( here, calendar::turn );
        veh_ptr->discharge_battery( here, 100000 );
        REQUIRE( veh_ptr->fuel_left( here, fuel_type_battery ) == 0 );
        WHEN( "24 hours pass" ) {
            veh_ptr->update_time( here, calendar::turn + 24_hours );
            int power = veh_ptr->fuel_left( here, fuel_type_battery );
            CHECK( power == Approx( 5259 ).margin( 1 ) );
        }
    }

    SECTION( "Summer day 1" ) {
        calendar::turn = calendar::turn_zero + calendar::season_length();
        veh_ptr->update_time( here, calendar::turn );
        veh_ptr->discharge_battery( here, 100000 );
        REQUIRE( veh_ptr->fuel_left( here, fuel_type_battery ) == 0 );
        WHEN( "24 hours pass" ) {
            veh_ptr->update_time( here, calendar::turn + 24_hours );
            int power = veh_ptr->fuel_left( here, fuel_type_battery );
            CHECK( power == Approx( 7925 ).margin( 1 ) );
        }
    }

    SECTION( "Autum day 1" ) {
        calendar::turn = calendar::turn_zero + 2 * calendar::season_length();
        veh_ptr->update_time( here, calendar::turn );
        veh_ptr->discharge_battery( here, 100000 );
        REQUIRE( veh_ptr->fuel_left( here, fuel_type_battery ) == 0 );
        WHEN( "24 hours pass" ) {
            veh_ptr->update_time( here, calendar::turn + 24_hours );
            int power = veh_ptr->fuel_left( here, fuel_type_battery );
            CHECK( power == Approx( 5138 ).margin( 1 ) );
        }
    }

    SECTION( "Winter day 1" ) {
        calendar::turn = calendar::turn_zero + 3 * calendar::season_length();
        veh_ptr->update_time( here, calendar::turn );
        veh_ptr->discharge_battery( here, 100000 );
        REQUIRE( veh_ptr->fuel_left( here, fuel_type_battery ) == 0 );
        WHEN( "24 hours pass" ) {
            veh_ptr->update_time( here, calendar::turn + 24_hours );
            int power = veh_ptr->fuel_left( here, fuel_type_battery );
            CHECK( power == Approx( 2137 ).margin( 1 ) );
        }
    }
}

TEST_CASE( "maximum_reverse_velocity", "[vehicle][power][reverse]" )
{
    clear_map();
    reset_player();
    build_test_map( ter_id( "t_pavement" ) );
    clear_vehicles();
    map &here = get_map();

    GIVEN( "a scooter with combustion engine and charged battery" ) {
        const tripoint_bub_ms origin{ 10, 0, 0 };
        vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_scooter_test, origin, 0_degrees, 0, 0 );
        REQUIRE( veh_ptr != nullptr );
        veh_ptr->charge_battery( here, 450 );
        REQUIRE( veh_ptr->fuel_left( here, fuel_type_battery ) == 450 );

        WHEN( "the engine is started" ) {
            veh_ptr->start_engines( here );

            THEN( "it can go in both forward and reverse" ) {
                int max_fwd = veh_ptr->max_velocity( here, false );
                int max_rev = veh_ptr->max_reverse_velocity( here, false );

                CHECK( max_rev < 0 );
                CHECK( max_fwd > 0 );

                AND_THEN( "its maximum reverse velocity is 1/4 of the maximum forward velocity" ) {
                    CHECK( std::abs( max_fwd / max_rev ) == 4 );
                }
            }

        }
    }

    GIVEN( "a scooter with an electric motor and charged battery" ) {
        const tripoint_bub_ms origin{ 15, 0, 0 };
        vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_scooter_electric_test, origin,
                                             0_degrees, 0, 0 );
        REQUIRE( veh_ptr != nullptr );
        veh_ptr->charge_battery( here, 5000 );
        REQUIRE( veh_ptr->fuel_left( here, fuel_type_battery ) == 5000 );

        WHEN( "the engine is started" ) {
            veh_ptr->start_engines( here );

            THEN( "it can go in both forward and reverse" ) {
                int max_fwd = veh_ptr->max_velocity( here, false );
                int max_rev = veh_ptr->max_reverse_velocity( here, false );

                CHECK( max_rev < 0 );
                CHECK( max_fwd > 0 );

                AND_THEN( "its maximum reverse velocity is equal to maximum forward velocity" ) {
                    CHECK( std::abs( max_rev ) == std::abs( max_fwd ) );
                }
            }
        }
    }
}

