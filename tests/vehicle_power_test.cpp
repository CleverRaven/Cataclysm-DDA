#include <cstdlib>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "debug.h"
#include "enums.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "item.h"
#include "json.h"
#include "json_loader.h"
#include "level_cache.h"
#include "map.h"
#include "map_helpers.h"
#include "map_scale_constants.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "power_network.h"
#include "ret_val.h"
#include "safe_reference.h"
#include "type_id.h"
#include "units.h"
#include "weather.h"
#include "veh_appliance.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather_type.h"

static const efftype_id effect_blind( "blind" );

static const itype_id fuel_type_battery( "battery" );
static const itype_id itype_ground_solar_panel( "ground_solar_panel" );
static const itype_id itype_test_high_drain_lamp( "test_high_drain_lamp" );
static const itype_id itype_test_power_cord( "test_power_cord" );
static const itype_id itype_test_power_cord_25_loss( "test_power_cord_25_loss" );
static const itype_id itype_test_standing_lamp( "test_standing_lamp" );
static const itype_id itype_test_storage_battery( "test_storage_battery" );

static const vpart_id vpart_ap_ground_solar_panel( "ap_ground_solar_panel" );
static const vpart_id vpart_ap_test_high_drain_lamp( "ap_test_high_drain_lamp" );
static const vpart_id vpart_ap_test_standing_lamp( "ap_test_standing_lamp" );
static const vpart_id vpart_ap_test_storage_battery( "ap_test_storage_battery" );
static const vpart_id vpart_frame( "frame" );
static const vpart_id vpart_small_storage_battery( "small_storage_battery" );

static const vproto_id vehicle_prototype_none( "none" );
static const vproto_id vehicle_prototype_scooter_electric_test( "scooter_electric_test" );
static const vproto_id vehicle_prototype_scooter_test( "scooter_test" );
static const vproto_id vehicle_prototype_solar_panel_test( "solar_panel_test" );
static const vproto_id vehicle_prototype_water_wheel_test( "water_wheel_test" );
static const vproto_id vehicle_prototype_wind_turbine_test( "wind_turbine_test" );

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
    clear_map_without_vision();
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

static void connect_power_cord( const tripoint_bub_ms &src_pos, const tripoint_bub_ms &dst_pos )
{
    map &here = get_map();
    const optional_vpart_position target_vp = here.veh_at( dst_pos );
    const optional_vpart_position source_vp = here.veh_at( src_pos );
    REQUIRE( target_vp.has_value() );
    REQUIRE( source_vp.has_value() );
    REQUIRE( &target_vp->vehicle() != &source_vp->vehicle() );

    item cord( itype_test_power_cord );
    ret_val<void> result = cord.link_to( target_vp, source_vp, link_state::vehicle_port );
    REQUIRE( result.success() );
}

// Helper: find the index of the first part with ENABLED_DRAINS_EPOWER on a vehicle
static int find_consumer_part_idx( vehicle &veh )
{
    for( const vpart_reference &vp : veh.get_all_parts() ) {
        if( vp.info().has_flag( "ENABLED_DRAINS_EPOWER" ) ) {
            return vp.part_index();
        }
    }
    return -1;
}

TEST_CASE( "power_parts_sets_power_disabled_flag_on_deficit", "[vehicle][power][grid]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    Character &player_character = get_player_character();

    const tripoint_bub_ms battery_pos( HALF_MAPSIZE_X + 2, HALF_MAPSIZE_Y + 2, 0 );
    const tripoint_bub_ms lamp_pos( HALF_MAPSIZE_X + 4, HALF_MAPSIZE_Y + 2, 0 );

    // Use the 1 kW lamp so power_to_energy_bat produces a non-zero delta
    // deterministically (1 kW * 1s = 1 kJ, roll_remainder(1.0) = 1 always).
    std::optional<item> battery_item( itype_test_storage_battery );
    std::optional<item> lamp_item( itype_test_high_drain_lamp );
    place_appliance( here, battery_pos, vpart_ap_test_storage_battery, player_character, battery_item );
    place_appliance( here, lamp_pos, vpart_ap_test_high_drain_lamp, player_character, lamp_item );

    connect_power_cord( battery_pos, lamp_pos );

    vehicle &lamp_veh = here.veh_at( lamp_pos )->vehicle();
    vehicle &bat_veh = here.veh_at( battery_pos )->vehicle();
    const int consumer_idx = find_consumer_part_idx( lamp_veh );
    REQUIRE( consumer_idx != -1 );

    // Drain battery completely
    bat_veh.discharge_battery( here, 100000000 );
    REQUIRE( bat_veh.fuel_left( here, fuel_type_battery ) == 0 );

    // Force the consumer enabled so power_parts() can disable it
    lamp_veh.part( consumer_idx ).enabled = true;
    lamp_veh.part( consumer_idx ).power_disabled = false;

    calendar::turn += 1_turns;
    here.vehmove();

    // power_parts() should have disabled the consumer AND set the flag.
    // Note: resolve_appliance_grid_power() runs after but won't re-enable
    // because the battery is at 0.
    CHECK_FALSE( lamp_veh.part( consumer_idx ).enabled );
    CHECK( lamp_veh.part( consumer_idx ).power_disabled );
}

TEST_CASE( "grid_power_resolution_reenables_appliances", "[vehicle][power][grid]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    Character &player_character = get_player_character();

    const tripoint_bub_ms battery_pos( HALF_MAPSIZE_X + 2, HALF_MAPSIZE_Y + 2, 0 );
    const tripoint_bub_ms lamp_pos( HALF_MAPSIZE_X + 4, HALF_MAPSIZE_Y + 2, 0 );

    std::optional<item> battery_item( itype_test_storage_battery );
    std::optional<item> lamp_item( itype_test_standing_lamp );
    place_appliance( here, battery_pos, vpart_ap_test_storage_battery, player_character, battery_item );
    place_appliance( here, lamp_pos, vpart_ap_test_standing_lamp, player_character, lamp_item );

    connect_power_cord( battery_pos, lamp_pos );

    vehicle &lamp_veh = here.veh_at( lamp_pos )->vehicle();
    vehicle &bat_veh = here.veh_at( battery_pos )->vehicle();
    const int consumer_idx = find_consumer_part_idx( lamp_veh );
    REQUIRE( consumer_idx != -1 );

    // Simulate the state after power_parts() deficit: consumer disabled with flag set.
    // (The deficit fires stochastically via roll_remainder for small consumers like -20W,
    // so we set the state directly rather than waiting for random rounding.)
    lamp_veh.part( consumer_idx ).enabled = false;
    lamp_veh.part( consumer_idx ).power_disabled = true;

    // Battery has charge -- resolution should re-enable the consumer
    REQUIRE( bat_veh.fuel_left( here, fuel_type_battery ) > 0 );

    calendar::turn += 1_turns;
    here.vehmove();
    CHECK( lamp_veh.part( consumer_idx ).enabled );
    CHECK_FALSE( lamp_veh.part( consumer_idx ).power_disabled );
}

TEST_CASE( "grid_power_no_recovery_without_power", "[vehicle][power][grid]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    Character &player_character = get_player_character();

    const tripoint_bub_ms battery_pos( HALF_MAPSIZE_X + 2, HALF_MAPSIZE_Y + 2, 0 );
    const tripoint_bub_ms lamp_pos( HALF_MAPSIZE_X + 4, HALF_MAPSIZE_Y + 2, 0 );

    std::optional<item> battery_item( itype_test_storage_battery );
    std::optional<item> lamp_item( itype_test_standing_lamp );
    place_appliance( here, battery_pos, vpart_ap_test_storage_battery, player_character, battery_item );
    place_appliance( here, lamp_pos, vpart_ap_test_standing_lamp, player_character, lamp_item );

    connect_power_cord( battery_pos, lamp_pos );

    vehicle &lamp_veh = here.veh_at( lamp_pos )->vehicle();
    vehicle &bat_veh = here.veh_at( battery_pos )->vehicle();
    const int consumer_idx = find_consumer_part_idx( lamp_veh );
    REQUIRE( consumer_idx != -1 );

    // Simulate power deficit state
    lamp_veh.part( consumer_idx ).enabled = false;
    lamp_veh.part( consumer_idx ).power_disabled = true;

    // Drain all battery power
    bat_veh.discharge_battery( here, 100000000 );
    REQUIRE( bat_veh.fuel_left( here, fuel_type_battery ) == 0 );

    calendar::turn += 1_turns;
    here.vehmove();
    CHECK_FALSE( lamp_veh.part( consumer_idx ).enabled );
    CHECK( lamp_veh.part( consumer_idx ).power_disabled );

    // another turn, still no power
    calendar::turn += 1_turns;
    here.vehmove();
    CHECK_FALSE( lamp_veh.part( consumer_idx ).enabled );
    CHECK( lamp_veh.part( consumer_idx ).power_disabled );
}

TEST_CASE( "user_disabled_appliance_not_reenabled", "[vehicle][power][grid]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    Character &player_character = get_player_character();

    const tripoint_bub_ms battery_pos( HALF_MAPSIZE_X + 2, HALF_MAPSIZE_Y + 2, 0 );
    const tripoint_bub_ms lamp_pos( HALF_MAPSIZE_X + 4, HALF_MAPSIZE_Y + 2, 0 );

    std::optional<item> battery_item( itype_test_storage_battery );
    std::optional<item> lamp_item( itype_test_standing_lamp );
    place_appliance( here, battery_pos, vpart_ap_test_storage_battery, player_character, battery_item );
    place_appliance( here, lamp_pos, vpart_ap_test_standing_lamp, player_character, lamp_item );

    connect_power_cord( battery_pos, lamp_pos );

    vehicle &lamp_veh = here.veh_at( lamp_pos )->vehicle();
    const int consumer_idx = find_consumer_part_idx( lamp_veh );
    REQUIRE( consumer_idx != -1 );

    // user manually disables the lamp (power_disabled stays false)
    lamp_veh.part( consumer_idx ).enabled = false;
    lamp_veh.part( consumer_idx ).power_disabled = false;

    calendar::turn += 1_turns;
    here.vehmove();

    // resolution should not touch user-disabled parts
    CHECK_FALSE( lamp_veh.part( consumer_idx ).enabled );
    CHECK_FALSE( lamp_veh.part( consumer_idx ).power_disabled );
}

TEST_CASE( "non_appliance_vehicle_not_auto_recovered", "[vehicle][power][grid]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();

    // build a regular (non-appliance) vehicle with a frame, battery, and a consumer part
    const tripoint_bub_ms veh_pos( HALF_MAPSIZE_X + 2, HALF_MAPSIZE_Y + 2, 0 );
    vehicle *veh = here.add_vehicle( vehicle_prototype_none, veh_pos, 0_degrees, 0, 0 );
    REQUIRE( veh != nullptr );
    const int frame_idx = veh->install_part( here, point_rel_ms::zero, vpart_frame );
    REQUIRE( frame_idx != -1 );
    const int bat_idx = veh->install_part( here, point_rel_ms::zero, vpart_small_storage_battery );
    REQUIRE( bat_idx != -1 );
    veh->refresh();
    here.add_vehicle_to_cache( veh );

    // charge battery, then drain it
    veh->charge_battery( here, 500 );
    REQUIRE( veh->fuel_left( here, fuel_type_battery ) > 0 );

    // the small_storage_battery vehicle is NOT an appliance
    REQUIRE_FALSE( veh->is_appliance() );

    // manually simulate a power deficit disable
    veh->part( bat_idx ).enabled = false;
    veh->part( bat_idx ).power_disabled = true;

    calendar::turn += 1_turns;
    here.vehmove();

    // resolution should skip non-appliance vehicles
    CHECK_FALSE( veh->part( bat_idx ).enabled );
}

TEST_CASE( "grid_power_cross_turn_recovery", "[vehicle][power][grid]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    Character &player_character = get_player_character();

    const tripoint_bub_ms battery_pos( HALF_MAPSIZE_X + 2, HALF_MAPSIZE_Y + 2, 0 );
    const tripoint_bub_ms lamp_pos( HALF_MAPSIZE_X + 4, HALF_MAPSIZE_Y + 2, 0 );

    std::optional<item> battery_item( itype_test_storage_battery );
    std::optional<item> lamp_item( itype_test_standing_lamp );
    place_appliance( here, battery_pos, vpart_ap_test_storage_battery, player_character, battery_item );
    place_appliance( here, lamp_pos, vpart_ap_test_standing_lamp, player_character, lamp_item );

    connect_power_cord( battery_pos, lamp_pos );

    vehicle &lamp_veh = here.veh_at( lamp_pos )->vehicle();
    vehicle &bat_veh = here.veh_at( battery_pos )->vehicle();
    const int consumer_idx = find_consumer_part_idx( lamp_veh );
    REQUIRE( consumer_idx != -1 );

    // Simulate power deficit state with drained battery
    bat_veh.discharge_battery( here, 100000000 );
    REQUIRE( bat_veh.fuel_left( here, fuel_type_battery ) == 0 );
    lamp_veh.part( consumer_idx ).enabled = false;
    lamp_veh.part( consumer_idx ).power_disabled = true;

    // second turn, still no power -- flag persists
    calendar::turn += 1_turns;
    here.vehmove();
    CHECK_FALSE( lamp_veh.part( consumer_idx ).enabled );
    CHECK( lamp_veh.part( consumer_idx ).power_disabled );

    // third turn, charge battery -- should recover
    bat_veh.charge_battery( here, 1000 );
    REQUIRE( bat_veh.fuel_left( here, fuel_type_battery ) > 0 );
    calendar::turn += 1_turns;
    here.vehmove();
    CHECK( lamp_veh.part( consumer_idx ).enabled );
    CHECK_FALSE( lamp_veh.part( consumer_idx ).power_disabled );
}

TEST_CASE( "cable_survives_target_outside_reality_bubble", "[vehicle][power][grid]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    Character &player_character = get_player_character();

    // Place an appliance to connect to
    const tripoint_bub_ms battery_pos( HALF_MAPSIZE_X + 2, HALF_MAPSIZE_Y + 2, 0 );
    std::optional<item> battery_item( itype_test_storage_battery );
    place_appliance( here, battery_pos, vpart_ap_test_storage_battery,
                     player_character, battery_item );

    const optional_vpart_position target_vp = here.veh_at( battery_pos );
    REQUIRE( target_vp.has_value() );

    // Create a cord and link one end to the appliance (single-ended link).
    // This simulates a tool/item connected to a vehicle via cable.
    item cord( itype_test_power_cord );
    ret_val<void> result = cord.link_to( target_vp, link_state::vehicle_port );
    REQUIRE( result.success() );
    REQUIRE( cord.has_link_data() );
    REQUIRE( cord.link().target == link_state::vehicle_port );

    // Where the cord "sits" on the map
    const tripoint_bub_ms cord_pos( HALF_MAPSIZE_X + 4, HALF_MAPSIZE_Y + 2, 0 );
    // Initialize s_bub_pos so process_link doesn't trigger a length check
    cord.link().s_bub_pos = cord_pos;

    // Drop the cord on the map so process() works correctly
    here.add_item( cord_pos, cord );
    item &map_cord = here.i_at( cord_pos ).only_item();
    REQUIRE( map_cord.has_link_data() );
    REQUIRE( map_cord.link().target == link_state::vehicle_port );

    SECTION( "cable disconnects when in-bounds target vehicle is gone" ) {
        // Set target to an in-bounds position with no vehicle, expire the reference.
        // This simulates a genuinely missing vehicle -- should disconnect.
        tripoint_bub_ms empty_pos( HALF_MAPSIZE_X + 10, HALF_MAPSIZE_Y + 10, 0 );
        map_cord.link().t_abs_pos = here.get_abs( empty_pos );
        map_cord.link().t_veh = safe_reference<vehicle>();

        map_cord.process( here, nullptr, cord_pos );

        CHECK_FALSE( map_cord.has_link_data() );
    }

    SECTION( "cable survives when target is out of bounds" ) {
        // Set target to an OOB position and expire the reference.
        // This simulates a vehicle whose submap was unloaded during a bubble
        // transition -- it still exists, just not accessible right now.
        tripoint_abs_ms oob_pos = here.get_abs( battery_pos ) +
                                  tripoint( MAPSIZE_X * 2, MAPSIZE_Y * 2, 0 );
        REQUIRE_FALSE( here.inbounds( oob_pos ) );

        map_cord.link().t_abs_pos = oob_pos;
        map_cord.link().t_veh = safe_reference<vehicle>();

        map_cord.process( here, nullptr, cord_pos );

        // Cable should still be connected -- target is just out of the bubble
        CHECK( map_cord.has_link_data() );
        CHECK( map_cord.link().target == link_state::vehicle_port );
    }

    SECTION( "OOB length check does not false-positive disconnect" ) {
        // Set target to OOB but keep the vehicle reference valid.
        // Move the cord position so length_check_needed triggers.
        // Stale OOB positions should not cause false over-extension.
        tripoint_abs_ms oob_pos = here.get_abs( battery_pos ) +
                                  tripoint( MAPSIZE_X * 2, MAPSIZE_Y * 2, 0 );
        REQUIRE_FALSE( here.inbounds( oob_pos ) );

        map_cord.link().t_abs_pos = oob_pos;
        // Keep t_veh valid (pointing to the real vehicle)
        // but force a length check by changing s_bub_pos
        tripoint_bub_ms moved_pos( HALF_MAPSIZE_X + 5, HALF_MAPSIZE_Y + 2, 0 );

        map_cord.process( here, nullptr, moved_pos );

        // Cable should survive -- stale OOB positions are unreliable for length
        CHECK( map_cord.has_link_data() );
        CHECK( map_cord.link().target == link_state::vehicle_port );
    }
}

TEST_CASE( "off_map_solar_catchup_generates_energy", "[vehicle][power][grid]" )
{
    clear_vehicles();
    reset_player();
    build_test_map( ter_id( "t_pavement" ) );
    map &here = get_map();

    const tripoint_bub_ms solar_origin{ 5, 5, 0 };
    vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_solar_panel_test, solar_origin,
                                         0_degrees, 0, 0 );
    REQUIRE( veh_ptr != nullptr );
    REQUIRE_FALSE( veh_ptr->solar_panels.empty() );
    scoped_weather_override clear_weather( WEATHER_CLEAR );

    // Set time to summer noon for maximum solar
    calendar::turn = calendar::turn_zero + calendar::season_length() + 1_days + 12_hours;
    veh_ptr->update_time( here, calendar::turn );

    // Drain battery fully
    veh_ptr->discharge_battery( here, 100000000 );
    REQUIRE( veh_ptr->fuel_left( here, fuel_type_battery ) == 0 );

    SECTION( "30 minutes of off-map solar matches on-map update_time" ) {
        // Run catchup for 30 minutes
        time_point future = calendar::turn + 30_minutes;
        int offmap_energy = veh_ptr->catchup_off_map_renewables( future );
        CHECK( offmap_energy > 0 );
        // Should produce roughly the same as update_time for the same interval.
        // The exact value may differ slightly because update_time uses
        // bub_part_pos while catchup uses abs_part_pos, but for outdoor tiles
        // on a flat map the outside check should agree.
        CHECK( offmap_energy == Approx( 425 ).margin( 10 ) );
    }

    SECTION( "underground vehicle produces no solar" ) {
        veh_ptr->sm_pos = tripoint_abs_sm( veh_ptr->sm_pos.xy(), -1 );
        time_point future = calendar::turn + 30_minutes;
        int offmap_energy = veh_ptr->catchup_off_map_renewables( future );
        CHECK( offmap_energy == 0 );
    }

    SECTION( "indoor solar panels do not generate" ) {
        // Rebuild map with indoor terrain (has TFLAG_INDOORS)
        clear_vehicles();
        build_test_map( ter_id( "t_thconc_floor" ) );
        vehicle *indoor_veh = here.add_vehicle( vehicle_prototype_solar_panel_test,
                                                solar_origin, 0_degrees, 0, 0 );
        REQUIRE( indoor_veh != nullptr );
        REQUIRE_FALSE( indoor_veh->solar_panels.empty() );

        calendar::turn = calendar::turn_zero + calendar::season_length() + 1_days + 12_hours;
        indoor_veh->update_time( here, calendar::turn );

        time_point future = calendar::turn + 30_minutes;
        int offmap_energy = indoor_veh->catchup_off_map_renewables( future );
        CHECK( offmap_energy == 0 );
    }
}

TEST_CASE( "off_map_catchup_prevents_double_charge", "[vehicle][power][grid]" )
{
    clear_vehicles();
    reset_player();
    build_test_map( ter_id( "t_pavement" ) );
    map &here = get_map();

    const tripoint_bub_ms solar_origin{ 5, 5, 0 };
    vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_solar_panel_test, solar_origin,
                                         0_degrees, 0, 0 );
    REQUIRE( veh_ptr != nullptr );
    scoped_weather_override clear_weather( WEATHER_CLEAR );

    calendar::turn = calendar::turn_zero + calendar::season_length() + 1_days + 12_hours;
    veh_ptr->update_time( here, calendar::turn );
    veh_ptr->discharge_battery( here, 100000000 );
    REQUIRE( veh_ptr->fuel_left( here, fuel_type_battery ) == 0 );

    // Simulate off-map catchup advancing last_update
    time_point future = calendar::turn + 30_minutes;
    int offmap_energy = veh_ptr->catchup_off_map_renewables( future );
    REQUIRE( offmap_energy > 0 );

    // Now simulate on-map re-entry: update_time should produce nothing
    // because last_update was already advanced by catchup
    veh_ptr->update_time( here, future );
    // Battery should still be zero (catchup returned energy but didn't charge)
    CHECK( veh_ptr->fuel_left( here, fuel_type_battery ) == 0 );
}

TEST_CASE( "off_map_catchup_skips_short_intervals", "[vehicle][power][grid]" )
{
    clear_vehicles();
    reset_player();
    build_test_map( ter_id( "t_pavement" ) );
    map &here = get_map();

    const tripoint_bub_ms solar_origin{ 5, 5, 0 };
    vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_solar_panel_test, solar_origin,
                                         0_degrees, 0, 0 );
    REQUIRE( veh_ptr != nullptr );
    scoped_weather_override clear_weather( WEATHER_CLEAR );

    calendar::turn = calendar::turn_zero + calendar::season_length() + 1_days + 12_hours;
    veh_ptr->update_time( here, calendar::turn );

    // Less than 1 minute should return 0
    int energy = veh_ptr->catchup_off_map_renewables( calendar::turn + 30_seconds );
    CHECK( energy == 0 );
}

TEST_CASE( "power_network_rebuild_matches_grid", "[vehicle][power][grid]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    Character &player_character = get_player_character();

    const tripoint_bub_ms battery_pos( HALF_MAPSIZE_X + 2, HALF_MAPSIZE_Y + 2, 0 );
    const tripoint_bub_ms lamp_pos( HALF_MAPSIZE_X + 4, HALF_MAPSIZE_Y + 2, 0 );

    std::optional<item> battery_item( itype_test_storage_battery );
    std::optional<item> lamp_item( itype_test_standing_lamp );
    place_appliance( here, battery_pos, vpart_ap_test_storage_battery, player_character, battery_item );
    place_appliance( here, lamp_pos, vpart_ap_test_standing_lamp, player_character, lamp_item );

    connect_power_cord( battery_pos, lamp_pos );

    vehicle &bat_veh = here.veh_at( battery_pos )->vehicle();

    // BFS from battery should find both vehicles
    std::map<vehicle *, float> grid = bat_veh.search_connected_vehicles( here );
    REQUIRE( grid.size() == 2 );

    // Rebuild power networks via vehmove
    calendar::turn += 1_turns;
    here.vehmove();

    power_network_manager &pnm = g->power_networks();
    const auto &networks = pnm.all_networks();
    REQUIRE( networks.size() == 1 );

    const power_network &net = networks.begin()->second;
    CHECK( net.nodes.size() == grid.size() );
    CHECK( net.last_resolved == calendar::turn );

    // Every vehicle in the BFS should appear as a node
    for( const auto &[veh, loss] : grid ) {
        const power_network *found = pnm.find_network_at( veh->pos_abs() );
        CHECK( found != nullptr );
        CHECK( found->id == net.id );
    }
}

TEST_CASE( "power_network_serialization_round_trip", "[vehicle][power][grid]" )
{
    // Load a hand-crafted network state from raw JSON
    power_network_manager original;
    {
        std::string raw = R"({
            "next_id": 5,
            "networks": [{
                "id": 3,
                "last_resolved": 604800,
                "root_pos": [100, 200, 0],
                "nodes": [{
                    "pos": [100, 200, 0],
                    "solar": "400 W",
                    "wind": "0 W",
                    "water": "0 W",
                    "consumption": "-50 W",
                    "battery_cap": 5000
                }]
            }]
        })";
        JsonValue jv = json_loader::from_string( raw );
        JsonObject jo = jv;
        original.deserialize( jo );
    }

    // Serialize via the manager's own method
    std::ostringstream os;
    JsonOut jsout( os );
    original.serialize( jsout );

    // Deserialize into a new manager
    power_network_manager loaded;
    JsonValue jv = json_loader::from_string( os.str() );
    JsonObject jo = jv;
    loaded.deserialize( jo );

    const auto &orig_nets = original.all_networks();
    const auto &load_nets = loaded.all_networks();
    REQUIRE( orig_nets.size() == load_nets.size() );

    const power_network &orig_net = orig_nets.begin()->second;
    const power_network &load_net = load_nets.begin()->second;
    CHECK( orig_net.id == load_net.id );
    CHECK( orig_net.last_resolved == load_net.last_resolved );
    CHECK( orig_net.root_position == load_net.root_position );
    REQUIRE( orig_net.nodes.size() == load_net.nodes.size() );
    CHECK( orig_net.nodes[0].position == load_net.nodes[0].position );
    CHECK( orig_net.nodes[0].rated_solar == load_net.nodes[0].rated_solar );
    CHECK( orig_net.nodes[0].rated_consumption == load_net.nodes[0].rated_consumption );
    CHECK( orig_net.nodes[0].battery_capacity_kJ == load_net.nodes[0].battery_capacity_kJ );
}

TEST_CASE( "power_network_old_save_migration", "[vehicle][power][grid]" )
{
    // An empty manager (simulating an old save with no power_networks key)
    // should work fine -- first vehmove builds the networks from scratch.
    power_network_manager pnm;
    CHECK( pnm.all_networks().empty() );

    // A rebuild on empty state should assign fresh IDs starting from 1
    g->power_networks().clear();
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    Character &player_character = get_player_character();

    const tripoint_bub_ms battery_pos( HALF_MAPSIZE_X + 2, HALF_MAPSIZE_Y + 2, 0 );
    std::optional<item> battery_item( itype_test_storage_battery );
    place_appliance( here, battery_pos, vpart_ap_test_storage_battery, player_character, battery_item );

    calendar::turn += 1_turns;
    here.vehmove();

    const auto &networks = g->power_networks().all_networks();
    CHECK_FALSE( networks.empty() );
    // First network assigned should have id=1
    CHECK( networks.begin()->second.id == 1 );
}

TEST_CASE( "power_network_id_stability_across_rebuilds", "[vehicle][power][grid]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    Character &player_character = get_player_character();

    const tripoint_bub_ms battery_pos( HALF_MAPSIZE_X + 2, HALF_MAPSIZE_Y + 2, 0 );
    const tripoint_bub_ms lamp_pos( HALF_MAPSIZE_X + 4, HALF_MAPSIZE_Y + 2, 0 );

    std::optional<item> battery_item( itype_test_storage_battery );
    std::optional<item> lamp_item( itype_test_standing_lamp );
    place_appliance( here, battery_pos, vpart_ap_test_storage_battery, player_character, battery_item );
    place_appliance( here, lamp_pos, vpart_ap_test_standing_lamp, player_character, lamp_item );

    connect_power_cord( battery_pos, lamp_pos );

    // First rebuild
    calendar::turn += 1_turns;
    here.vehmove();

    power_network_manager &pnm = g->power_networks();
    REQUIRE( pnm.all_networks().size() == 1 );
    int first_id = pnm.all_networks().begin()->second.id;

    // Second rebuild -- same grid, should keep the same id
    calendar::turn += 1_turns;
    here.vehmove();

    REQUIRE( pnm.all_networks().size() == 1 );
    int second_id = pnm.all_networks().begin()->second.id;
    CHECK( first_id == second_id );

    // Third rebuild -- still stable
    calendar::turn += 1_turns;
    here.vehmove();
    REQUIRE( pnm.all_networks().size() == 1 );
    CHECK( pnm.all_networks().begin()->second.id == first_id );
}

TEST_CASE( "power_network_topology_mutation", "[vehicle][power][grid]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    Character &player_character = get_player_character();

    const tripoint_bub_ms bat_pos( HALF_MAPSIZE_X + 2, HALF_MAPSIZE_Y + 2, 0 );
    const tripoint_bub_ms lamp1_pos( HALF_MAPSIZE_X + 4, HALF_MAPSIZE_Y + 2, 0 );
    const tripoint_bub_ms lamp2_pos( HALF_MAPSIZE_X + 6, HALF_MAPSIZE_Y + 2, 0 );

    std::optional<item> battery_item( itype_test_storage_battery );
    std::optional<item> lamp1_item( itype_test_standing_lamp );
    std::optional<item> lamp2_item( itype_test_standing_lamp );
    place_appliance( here, bat_pos, vpart_ap_test_storage_battery, player_character, battery_item );
    place_appliance( here, lamp1_pos, vpart_ap_test_standing_lamp, player_character, lamp1_item );
    place_appliance( here, lamp2_pos, vpart_ap_test_standing_lamp, player_character, lamp2_item );

    // Connect battery to lamp1 only
    connect_power_cord( bat_pos, lamp1_pos );

    calendar::turn += 1_turns;
    here.vehmove();

    power_network_manager &pnm = g->power_networks();
    // lamp2 is disconnected -- should be its own network or not in the battery's network
    const power_network *bat_net = pnm.find_network_at(
                                       here.get_abs( bat_pos ) );
    REQUIRE( bat_net != nullptr );
    CHECK( bat_net->nodes.size() == 2 );

    const power_network *lamp2_net = pnm.find_network_at(
                                         here.get_abs( lamp2_pos ) );
    REQUIRE( lamp2_net != nullptr );
    // lamp2 should NOT be in the battery's network
    CHECK( lamp2_net->id != bat_net->id );

    SECTION( "connecting lamp2 adds it to the network" ) {
        connect_power_cord( lamp1_pos, lamp2_pos );

        calendar::turn += 1_turns;
        here.vehmove();

        const power_network *updated_net = pnm.find_network_at(
                                               here.get_abs( bat_pos ) );
        REQUIRE( updated_net != nullptr );
        CHECK( updated_net->nodes.size() == 3 );

        // lamp2 should now be in the same network
        const power_network *lamp2_updated = pnm.find_network_at(
                here.get_abs( lamp2_pos ) );
        REQUIRE( lamp2_updated != nullptr );
        CHECK( lamp2_updated->id == updated_net->id );
    }

    SECTION( "removing a vehicle splits the network" ) {
        // Connect all three: bat -> lamp1 -> lamp2
        connect_power_cord( lamp1_pos, lamp2_pos );

        calendar::turn += 1_turns;
        here.vehmove();

        const power_network *full_net = pnm.find_network_at(
                                            here.get_abs( bat_pos ) );
        REQUIRE( full_net != nullptr );
        REQUIRE( full_net->nodes.size() == 3 );
        int original_id = full_net->id;

        // Remove lamp2 from the map
        here.destroy_vehicle( &here.veh_at( lamp2_pos )->vehicle() );

        calendar::turn += 1_turns;
        here.vehmove();

        // bat + lamp1 should remain as a network keeping the original id
        const power_network *remaining = pnm.find_network_at(
                                             here.get_abs( bat_pos ) );
        REQUIRE( remaining != nullptr );
        CHECK( remaining->nodes.size() == 2 );
        CHECK( remaining->id == original_id );

        // lamp2 is gone -- no network at that position
        CHECK( pnm.find_network_at( here.get_abs( lamp2_pos ) ) == nullptr );
    }
}

TEST_CASE( "power_network_rated_values_populated", "[vehicle][power][grid]" )
{
    clear_vehicles();
    reset_player();
    build_test_map( ter_id( "t_pavement" ) );
    map &here = get_map();

    const tripoint_bub_ms solar_origin{ 5, 5, 0 };
    vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_solar_panel_test, solar_origin,
                                         0_degrees, 0, 0 );
    REQUIRE( veh_ptr != nullptr );
    REQUIRE_FALSE( veh_ptr->solar_panels.empty() );

    calendar::turn += 1_turns;
    here.vehmove();

    power_network_manager &pnm = g->power_networks();
    const power_network *net = pnm.find_network_at( veh_ptr->pos_abs() );
    REQUIRE( net != nullptr );
    REQUIRE( net->nodes.size() == 1 );

    // The solar panel test vehicle has solar panels, so rated_solar should be > 0
    CHECK( net->nodes[0].rated_solar > 0_W );
    // And it should match the vehicle's rated helper
    CHECK( net->nodes[0].rated_solar == veh_ptr->rated_solar_epower() );
}

TEST_CASE( "off_map_solar_charges_remote_battery_through_cable", "[vehicle][power][grid]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    Character &player_character = get_player_character();
    scoped_weather_override clear_weather( WEATHER_CLEAR );

    // Place battery appliance, lamp appliance, and solar panel appliance
    const tripoint_bub_ms battery_pos( HALF_MAPSIZE_X + 2, HALF_MAPSIZE_Y + 2, 0 );
    const tripoint_bub_ms lamp_pos( HALF_MAPSIZE_X + 4, HALF_MAPSIZE_Y + 2, 0 );
    // test_power_cord has max_charges: 3, so max 3-tile range between endpoints
    const tripoint_bub_ms solar_pos( HALF_MAPSIZE_X + 5, HALF_MAPSIZE_Y + 2, 0 );

    std::optional<item> battery_item( itype_test_storage_battery );
    std::optional<item> lamp_item( itype_test_standing_lamp );
    // ground_solar_panel has 4.8x epower of a basic solar panel
    std::optional<item> solar_item( itype_ground_solar_panel );
    place_appliance( here, battery_pos, vpart_ap_test_storage_battery, player_character, battery_item );
    place_appliance( here, lamp_pos, vpart_ap_test_standing_lamp, player_character, lamp_item );
    place_appliance( here, solar_pos, vpart_ap_ground_solar_panel, player_character,
                     solar_item );

    vehicle *solar_veh = veh_pointer_or_null( here.veh_at( solar_pos ) );
    REQUIRE( solar_veh != nullptr );
    REQUIRE_FALSE( solar_veh->solar_panels.empty() );

    // Connect: battery <-> lamp, battery <-> solar
    connect_power_cord( battery_pos, lamp_pos );
    connect_power_cord( battery_pos, solar_pos );

    // Set time to summer noon for maximum solar
    calendar::turn = calendar::turn_zero + calendar::season_length() + 1_days + 12_hours;
    solar_veh->update_time( here, calendar::turn );

    // Drain all batteries
    vehicle &bat_veh = here.veh_at( battery_pos )->vehicle();
    bat_veh.discharge_battery( here, 100000000 );
    solar_veh->discharge_battery( here, 100000000 );
    REQUIRE( bat_veh.fuel_left( here, fuel_type_battery ) == 0 );
    REQUIRE( solar_veh->fuel_left( here, fuel_type_battery ) == 0 );

    // Set up deficit-disabled lamp for ordering test
    vehicle &lamp_veh = here.veh_at( lamp_pos )->vehicle();
    for( const vpart_reference &vpr : lamp_veh.get_all_parts() ) {
        if( vpr.info().has_flag( VPFLAG_ENABLED_DRAINS_EPOWER ) ) {
            lamp_veh.part( vpr.part_index() ).enabled = false;
            lamp_veh.part( vpr.part_index() ).power_disabled = true;
        }
    }

    // Advance time by 30 minutes for catchup to produce energy
    calendar::turn += 30_minutes;

    // Fake the solar vehicle as off-map by removing it from level_cache
    level_cache &cache = here.access_cache( 0 );
    cache.vehicle_list.erase( solar_veh );

    SECTION( "off-map solar charges grid battery" ) {
        here.vehmove();
        CHECK( bat_veh.fuel_left( here, fuel_type_battery ) > 0 );
    }

    SECTION( "ordering: off-map charge enables deficit-disabled appliance" ) {
        // vehmove runs resolve_off_map_grid_generation before resolve_appliance_grid_power.
        // If ordering were reversed, battery would still be 0 when recovery checks,
        // so the lamp would stay disabled.
        here.vehmove();

        bool any_reenabled = false;
        for( const vpart_reference &vpr : lamp_veh.get_all_parts() ) {
            if( vpr.info().has_flag( VPFLAG_ENABLED_DRAINS_EPOWER ) ) {
                if( lamp_veh.part( vpr.part_index() ).enabled &&
                    !lamp_veh.part( vpr.part_index() ).power_disabled ) {
                    any_reenabled = true;
                }
            }
        }
        CHECK( any_reenabled );
    }

    SECTION( "cable loss applied to off-map charge" ) {
        // Get energy that the solar vehicle would produce with no cable loss
        time_point future = calendar::turn;
        vehicle *fresh_solar = here.add_vehicle( vehicle_prototype_solar_panel_test,
                               tripoint_bub_ms( HALF_MAPSIZE_X + 10, HALF_MAPSIZE_Y + 10, 0 ),
                               0_degrees, 0, 0 );
        REQUIRE( fresh_solar != nullptr );
        fresh_solar->update_time( here, calendar::turn - 30_minutes );
        int raw_energy = fresh_solar->catchup_off_map_renewables( future );
        REQUIRE( raw_energy > 0 );

        // Now run the grid path -- battery gets charged through cable (with loss)
        here.vehmove();
        int grid_charge = bat_veh.fuel_left( here, fuel_type_battery );

        // The grid charge should be less than raw energy due to cable loss
        // (test_power_cord has some loss factor)
        CHECK( grid_charge > 0 );
        CHECK( grid_charge <= raw_energy );
    }
}

TEST_CASE( "off_map_wind_turbine_catchup", "[vehicle][power][grid]" )
{
    clear_vehicles();
    reset_player();
    build_test_map( ter_id( "t_pavement" ) );
    map &here = get_map();
    scoped_weather_override clear_weather( WEATHER_CLEAR );

    const tripoint_bub_ms origin{ 5, 5, 0 };
    vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_wind_turbine_test, origin,
                                         0_degrees, 0, 0 );
    REQUIRE( veh_ptr != nullptr );
    REQUIRE_FALSE( veh_ptr->wind_turbines.empty() );

    calendar::turn = calendar::turn_zero + calendar::season_length() + 1_days + 12_hours;
    veh_ptr->update_time( here, calendar::turn );

    // Set wind speed high enough to generate
    get_weather().windspeed = 30;
    get_weather().winddirection = 0;

    veh_ptr->discharge_battery( here, 100000000 );
    REQUIRE( veh_ptr->fuel_left( here, fuel_type_battery ) == 0 );

    time_point future = calendar::turn + 30_minutes;
    int energy = veh_ptr->catchup_off_map_renewables( future );
    CHECK( energy > 0 );
}

TEST_CASE( "off_map_water_wheel_catchup", "[vehicle][power][grid]" )
{
    clear_vehicles();
    reset_player();
    // Start with pavement, then place flowing water under the wheel
    build_test_map( ter_id( "t_pavement" ) );
    map &here = get_map();

    const tripoint_bub_ms origin{ 5, 5, 0 };
    vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_water_wheel_test, origin,
                                         0_degrees, 0, 0 );
    REQUIRE( veh_ptr != nullptr );
    REQUIRE_FALSE( veh_ptr->water_wheels.empty() );

    // Set terrain under each water wheel part to flowing water (has CURRENT flag)
    for( const int idx : veh_ptr->water_wheels ) {
        const tripoint_bub_ms wpos = veh_ptr->bub_part_pos( here, veh_ptr->part( idx ) );
        here.ter_set( wpos, ter_id( "t_water_moving_sh" ) );
    }

    calendar::turn = calendar::turn_zero + calendar::season_length() + 1_days + 12_hours;
    veh_ptr->update_time( here, calendar::turn );

    veh_ptr->discharge_battery( here, 100000000 );
    REQUIRE( veh_ptr->fuel_left( here, fuel_type_battery ) == 0 );

    time_point future = calendar::turn + 30_minutes;
    int energy = veh_ptr->catchup_off_map_renewables( future );
    CHECK( energy > 0 );
}

TEST_CASE( "power_network_id_assignment_on_split", "[vehicle][power][grid]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    Character &player_character = get_player_character();

    // Place 3 appliances in a chain: A -- B -- C
    const tripoint_bub_ms pos_a( HALF_MAPSIZE_X + 2, HALF_MAPSIZE_Y + 2, 0 );
    const tripoint_bub_ms pos_b( HALF_MAPSIZE_X + 4, HALF_MAPSIZE_Y + 2, 0 );
    const tripoint_bub_ms pos_c( HALF_MAPSIZE_X + 6, HALF_MAPSIZE_Y + 2, 0 );

    std::optional<item> item_a( itype_test_storage_battery );
    std::optional<item> item_b( itype_test_standing_lamp );
    std::optional<item> item_c( itype_test_standing_lamp );
    place_appliance( here, pos_a, vpart_ap_test_storage_battery, player_character, item_a );
    place_appliance( here, pos_b, vpart_ap_test_standing_lamp, player_character, item_b );
    place_appliance( here, pos_c, vpart_ap_test_standing_lamp, player_character, item_c );

    connect_power_cord( pos_a, pos_b );
    connect_power_cord( pos_b, pos_c );

    // First rebuild: all 3 in one network
    calendar::turn += 1_turns;
    here.vehmove();

    power_network_manager &pnm = g->power_networks();
    const power_network *full = pnm.find_network_at( here.get_abs( pos_a ) );
    REQUIRE( full != nullptr );
    REQUIRE( full->nodes.size() == 3 );
    int original_id = full->id;

    // Remove the middle node B -- splits into A (alone) and C (alone)
    here.destroy_vehicle( &here.veh_at( pos_b )->vehicle() );

    calendar::turn += 1_turns;
    here.vehmove();

    // Should now have at least 2 networks (A and C are disconnected)
    const power_network *net_a = pnm.find_network_at( here.get_abs( pos_a ) );
    const power_network *net_c = pnm.find_network_at( here.get_abs( pos_c ) );
    REQUIRE( net_a != nullptr );
    REQUIRE( net_c != nullptr );
    CHECK( net_a->id != net_c->id );

    // One of them should have inherited the original ID (whichever was processed first
    // with 1 overlap node claims the old ID; the other gets a new one)
    CHECK( ( net_a->id == original_id || net_c->id == original_id ) );
}

TEST_CASE( "power_network_multiple_independent_grids", "[vehicle][power][grid]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    Character &player_character = get_player_character();

    // Grid 1: battery + lamp connected
    const tripoint_bub_ms bat1_pos( HALF_MAPSIZE_X + 2, HALF_MAPSIZE_Y + 2, 0 );
    const tripoint_bub_ms lamp1_pos( HALF_MAPSIZE_X + 4, HALF_MAPSIZE_Y + 2, 0 );
    // Grid 2: battery + lamp connected, far away from grid 1
    const tripoint_bub_ms bat2_pos( HALF_MAPSIZE_X + 12, HALF_MAPSIZE_Y + 2, 0 );
    const tripoint_bub_ms lamp2_pos( HALF_MAPSIZE_X + 14, HALF_MAPSIZE_Y + 2, 0 );

    std::optional<item> bat1_item( itype_test_storage_battery );
    std::optional<item> lamp1_item( itype_test_standing_lamp );
    std::optional<item> bat2_item( itype_test_storage_battery );
    std::optional<item> lamp2_item( itype_test_standing_lamp );
    place_appliance( here, bat1_pos, vpart_ap_test_storage_battery, player_character, bat1_item );
    place_appliance( here, lamp1_pos, vpart_ap_test_standing_lamp, player_character, lamp1_item );
    place_appliance( here, bat2_pos, vpart_ap_test_storage_battery, player_character, bat2_item );
    place_appliance( here, lamp2_pos, vpart_ap_test_standing_lamp, player_character, lamp2_item );

    connect_power_cord( bat1_pos, lamp1_pos );
    connect_power_cord( bat2_pos, lamp2_pos );

    calendar::turn += 1_turns;
    here.vehmove();

    power_network_manager &pnm = g->power_networks();

    // Each connected pair should be in its own network
    const power_network *net1 = pnm.find_network_at( here.get_abs( bat1_pos ) );
    const power_network *net2 = pnm.find_network_at( here.get_abs( bat2_pos ) );
    REQUIRE( net1 != nullptr );
    REQUIRE( net2 != nullptr );
    CHECK( net1->id != net2->id );
    CHECK( net1->nodes.size() == 2 );
    CHECK( net2->nodes.size() == 2 );

    // Members of each grid should be in the correct network
    const power_network *lamp1_net = pnm.find_network_at( here.get_abs( lamp1_pos ) );
    const power_network *lamp2_net = pnm.find_network_at( here.get_abs( lamp2_pos ) );
    REQUIRE( lamp1_net != nullptr );
    REQUIRE( lamp2_net != nullptr );
    CHECK( lamp1_net->id == net1->id );
    CHECK( lamp2_net->id == net2->id );
}
