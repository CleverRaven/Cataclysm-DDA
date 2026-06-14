#include <string>

#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "enums.h"
#include "item.h"
#include "map.h"
#include "type_id.h"
#include "units.h"
#include "weather.h"

static const flag_id json_flag_FROZEN( "FROZEN" );

static const itype_id itype_meat_cooked( "meat_cooked" );
static const itype_id itype_offal_canned( "offal_canned" );

static void set_map_temperature( units::temperature new_temperature )
{
    get_weather().temperature = new_temperature;
    get_weather().clear_temp_cache();
}

TEST_CASE( "Rate_of_rotting", "[rot]" )
{
    SECTION( "Passage of time" ) {
        // Item rot is a time duration.
        // At 65 F (18,3 C) item rots at rate of 1h/1h
        // So the level of rot should be about same as the item age
        // In preserving containers and in freezer the item should not rot at all
        // Item in freezer should not be frozen.

        // Items created at turn zero are handled differently, so ensure we're
        // not there.
        if( calendar::turn <= calendar::start_of_cataclysm ) {
            calendar::turn = calendar::start_of_cataclysm + 1_minutes;
        }

        item normal_item( itype_meat_cooked );

        item freeze_item( itype_offal_canned );

        item sealed_item( itype_offal_canned );
        sealed_item = sealed_item.in_its_container();

        set_map_temperature( units::from_fahrenheit( 65 ) ); // 18,3 C

        normal_item.process( get_map(), nullptr, tripoint_bub_ms::zero, 1, temperature_flag::NORMAL );
        sealed_item.process( get_map(), nullptr, tripoint_bub_ms::zero, 1, temperature_flag::NORMAL );
        freeze_item.process( get_map(), nullptr, tripoint_bub_ms::zero, 1, temperature_flag::NORMAL );

        // Item should exist with no rot when it is brand new
        CHECK( normal_item.get_rot() == 0_turns );
        CHECK( sealed_item.get_rot() == 0_turns );
        CHECK( freeze_item.get_rot() == 0_turns );

        INFO( "Initial turn: " << to_turn<int>( calendar::turn ) );

        calendar::turn += 20_minutes;
        sealed_item.process( get_map(), nullptr, tripoint_bub_ms::zero, 1, temperature_flag::NORMAL );
        normal_item.process( get_map(), nullptr, tripoint_bub_ms::zero, 1, temperature_flag::NORMAL );
        freeze_item.process( get_map(), nullptr, tripoint_bub_ms::zero, 1, temperature_flag::FREEZER );

        // After 20 minutes the normal item should have 20 minutes of rot
        CHECK( to_turns<int>( normal_item.get_rot() )
               == Approx( to_turns<int>( 20_minutes ) ).epsilon( 0.01 ) );
        // Item in freezer and in preserving container should have no rot
        CHECK( sealed_item.get_rot() == 0_turns );
        CHECK( freeze_item.get_rot() == 0_turns );

        // Move time 110 minutes
        calendar::turn += 110_minutes;
        sealed_item.process( get_map(), nullptr, tripoint_bub_ms::zero, 1, temperature_flag::NORMAL );
        freeze_item.process( get_map(), nullptr, tripoint_bub_ms::zero, 1, temperature_flag::FREEZER );
        // In freezer and in preserving container still should be no rot
        CHECK( sealed_item.get_rot() == 0_turns );
        CHECK( freeze_item.get_rot() == 0_turns );

        // The item in freezer should still not be frozen
        CHECK( !freeze_item.has_own_flag( json_flag_FROZEN ) );
    }
}

TEST_CASE( "Items_rot_away", "[rot]" )
{
    SECTION( "Item in reality bubble rots away" ) {
        // Item should rot away when it has 2x of its shelf life in rot.

        if( calendar::turn <= calendar::start_of_cataclysm ) {
            calendar::turn = calendar::start_of_cataclysm + 1_minutes;
        }

        item test_item( itype_meat_cooked );

        // Process item once to set all of its values.
        test_item.process( get_map(), nullptr, tripoint_bub_ms::zero, 1, temperature_flag::HEATER );

        // Set rot to >2 days and process again. process_temperature_rot should return true.
        calendar::turn += 20_minutes;
        test_item.mod_rot( 2_days );

        CHECK( test_item.process_temperature_rot( 1, tripoint_bub_ms::zero, get_map(), nullptr,
                temperature_flag::HEATER ) );
        INFO( "Rot: " << to_turns<int>( test_item.get_rot() ) );
    }
}

TEST_CASE( "Hourly_rotpoints", "[rot]" )
{
    item normal_item( itype_meat_cooked );

    // No rot below 32F/0C
    CHECK( normal_item.calc_hourly_rotpoints_at_temp( units::from_celsius( 0 ) ) == 0 );

    // Max rot above 145F/63C
    CHECK( normal_item.calc_hourly_rotpoints_at_temp( units::from_celsius( 63 ) ) == Approx(
               20364.67 ) );

    // Make sure no off by one error at the border
    CHECK( normal_item.calc_hourly_rotpoints_at_temp( units::from_celsius( 62 ) ) == Approx(
               20364.67 ) );

    // 3200 point/h at 65F/18C
    CHECK( normal_item.calc_hourly_rotpoints_at_temp( units::from_fahrenheit( 65 ) ) == Approx(
               3600 ) );

    // Doubles after +16F
    CHECK( normal_item.calc_hourly_rotpoints_at_temp( units::from_fahrenheit( 65 + 16 ) ) == Approx(
               3600.0 * 2 ) );

    // Halves after -16F
    CHECK( normal_item.calc_hourly_rotpoints_at_temp( units::from_fahrenheit( 65 - 16 ) ) == Approx(
               3600.0 / 2 ) );

    // Test the linear area. Halfway between 32F/9C (0 point/hour) and 38F/3C (1117.672 point/hour)
    CHECK( normal_item.calc_hourly_rotpoints_at_temp( units::from_fahrenheit( 35 ) ) == Approx(
               1117.672 / 2 ) );

    // Maximum rot at above 105 F
    CHECK( normal_item.calc_hourly_rotpoints_at_temp( units::from_fahrenheit( 107 ) ) == Approx(
               20364.67 ) );
}
