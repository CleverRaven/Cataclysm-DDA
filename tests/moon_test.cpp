#include <string>

#include "calendar.h"
#include "cata_catch.h"
#include "enum_conversions.h"

// MOON TESTS
//
// Summary of moon phases and moonlight
//
// In-game, the moon goes through several discrete phases based on season length. The phases are
// represented by an integer enum defined in calendar.h called `moon_phase` with these values:
//
// enum name                 light
//   0: MOON_NEW              1.00
//   1: MOON_WAXING_CRESCENT  3.25
//   2: MOON_HALF_MOON_WAXING 5.50
//   3: MOON_WAXING_GIBBOUS   7.75
//   4: MOON_FULL            10.00
//   5: MOON_WANING_GIBBOUS   7.75
//   6: MOON_HALF_MOON_WANING 5.50
//   7: MOON_WANING_CRESCENT  3.25
//
// During each phase, the amount of light emitted is is constant for the entire phase (indicated by
// the last column above). In other words, at no point does the moon emit 8.0 light.
//
// Phase changes take place at noon, so the change in the moon's light level occurs while the moon
// is "down" (so to speak). The relative positions of sun and moon are not actually modeled.
//
// At night, moonlight prevails. At dawn, light gradually goes from moonlight-level to
// sunlight-level over the course of the twilight time (1 hour), and at dusk the reverse occurs.

// ------
// Phases
// ------

// The full lunar cycle takes a duration defined by lunar_month(), by default the synodic month
// ~29.53 days when using the default season length of 91 days.
TEST_CASE( "moon_phase_repeats_once_per_synodic_month", "[calendar][moon][phase][synodic]" )
{
    calendar::set_season_length( 91 );
    REQUIRE( calendar::season_from_default_ratio() == Approx( 1.0f ) );

    // Synodic month is ~29.53 days
    REQUIRE( to_days<float>( lunar_month() ) == Approx( 29.53f ).margin( 0.001f ) );

    // We will check the tenth and hundredth cycle
    const time_duration ten_moons = 295_days;
    const time_duration hundred_moons = 2953_days;
    const time_point zero = calendar::turn_zero;

    // First day of cataclysm is a new moon, with full moon 14 days later
    SECTION( "first moon on day 0" ) {
        CHECK( get_moon_phase( zero ) == MOON_NEW );
        CHECK( get_moon_phase( zero + 14_days ) == MOON_FULL );
    }

    // The 10th moon, late in year 1 of cataclysm
    SECTION( "tenth moon on day 295" ) {
        CHECK( get_moon_phase( zero + ten_moons ) == MOON_NEW );
        CHECK( get_moon_phase( zero + ten_moons + 14_days ) == MOON_FULL );
    }

    // The 100th moon, about 8 years into the cataclysm
    SECTION( "hundredth moon on day 2953" ) {
        CHECK( get_moon_phase( zero + hundred_moons ) == MOON_NEW );
        CHECK( get_moon_phase( zero + hundred_moons + 14_days ) == MOON_FULL );
    }
}

// The user-defined season length influences the lunar month duration. At the 91-day default season
// length, one lunar (synodic) month is about 29.53 days. Longer or shorter seasons scale the lunar
// month accordingly.
//
TEST_CASE( "lunar_month_is_scaled_by_season_default_ratio", "[calendar][moon][month][season]" )
{
    const time_point zero = calendar::turn_zero;

    // Normal 91-day season length - 2 weeks from new moon to full moon
    WHEN( "seasons are 91 days" ) {
        calendar::set_season_length( 91 );
        REQUIRE( calendar::season_from_default_ratio() == Approx( 1.0f ) );

        THEN( "it takes 14 days from new moon to full moon" ) {
            CHECK( get_moon_phase( zero ) == MOON_NEW );
            CHECK( get_moon_phase( zero + 14_days ) == MOON_FULL );
        }
    }

    // Double normal season length - 4 weeks from new moon to full moon
    WHEN( "seasons are 182 days" ) {
        calendar::set_season_length( 182 );
        REQUIRE( calendar::season_from_default_ratio() == Approx( 2.0f ) );

        THEN( "it takes 28 days from new moon to full moon" ) {
            CHECK( get_moon_phase( zero ) == MOON_NEW );
            CHECK( get_moon_phase( zero + 28_days ) == MOON_FULL );
        }
    }

    // One-half normal season length - 1 week from new moon to full moon
    WHEN( "seasons are 45 days" ) {
        calendar::set_season_length( 45 );
        REQUIRE( calendar::season_from_default_ratio() == Approx( 0.5f ).margin( 0.01f ) );

        THEN( "it takes 7 days from new moon to full moon" ) {
            CHECK( get_moon_phase( zero ) == MOON_NEW );
            CHECK( get_moon_phase( zero + 7_days ) == MOON_FULL );
        }
    }

    calendar::set_season_length( 91 ); // Reset to default
    REQUIRE( calendar::season_from_default_ratio() == Approx( 1.0f ) );
}

// With 8 discrete phases during a 29-day period, each phase lasts three or four days.
// Over a 1-month period of 30 or 31 days, the moon should go through all 8 phases.
//
TEST_CASE( "moon_phases_each_day_for_a_month", "[calendar][moon][phase]" )
{
    calendar::set_season_length( 91 );
    REQUIRE( calendar::season_from_default_ratio() == Approx( 1.0f ) );

    const time_point zero = calendar::turn_zero;

    CHECK( get_moon_phase( zero + 0_days ) == MOON_NEW );
    CHECK( get_moon_phase( zero + 1_days ) == MOON_NEW );
    CHECK( get_moon_phase( zero + 2_days ) == MOON_WAXING_CRESCENT );
    CHECK( get_moon_phase( zero + 3_days ) == MOON_WAXING_CRESCENT );
    CHECK( get_moon_phase( zero + 4_days ) == MOON_WAXING_CRESCENT );
    CHECK( get_moon_phase( zero + 5_days ) == MOON_WAXING_CRESCENT );
    CHECK( get_moon_phase( zero + 6_days ) == MOON_HALF_MOON_WAXING );
    CHECK( get_moon_phase( zero + 7_days ) == MOON_HALF_MOON_WAXING );
    CHECK( get_moon_phase( zero + 8_days ) == MOON_HALF_MOON_WAXING );
    CHECK( get_moon_phase( zero + 9_days ) == MOON_HALF_MOON_WAXING );
    CHECK( get_moon_phase( zero + 10_days ) == MOON_WAXING_GIBBOUS );
    CHECK( get_moon_phase( zero + 11_days ) == MOON_WAXING_GIBBOUS );
    CHECK( get_moon_phase( zero + 12_days ) == MOON_WAXING_GIBBOUS );
    CHECK( get_moon_phase( zero + 13_days ) == MOON_FULL );
    CHECK( get_moon_phase( zero + 14_days ) == MOON_FULL );
    CHECK( get_moon_phase( zero + 15_days ) == MOON_FULL );
    CHECK( get_moon_phase( zero + 16_days ) == MOON_FULL );
    CHECK( get_moon_phase( zero + 17_days ) == MOON_WANING_GIBBOUS );
    CHECK( get_moon_phase( zero + 18_days ) == MOON_WANING_GIBBOUS );
    CHECK( get_moon_phase( zero + 19_days ) == MOON_WANING_GIBBOUS );
    CHECK( get_moon_phase( zero + 20_days ) == MOON_WANING_GIBBOUS );
    CHECK( get_moon_phase( zero + 21_days ) == MOON_HALF_MOON_WANING );
    CHECK( get_moon_phase( zero + 22_days ) == MOON_HALF_MOON_WANING );
    CHECK( get_moon_phase( zero + 23_days ) == MOON_HALF_MOON_WANING );
    CHECK( get_moon_phase( zero + 24_days ) == MOON_WANING_CRESCENT );
    CHECK( get_moon_phase( zero + 25_days ) == MOON_WANING_CRESCENT );
    CHECK( get_moon_phase( zero + 26_days ) == MOON_WANING_CRESCENT );
    CHECK( get_moon_phase( zero + 27_days ) == MOON_WANING_CRESCENT );
    CHECK( get_moon_phase( zero + 28_days ) == MOON_NEW );
    CHECK( get_moon_phase( zero + 29_days ) == MOON_NEW );
    CHECK( get_moon_phase( zero + 30_days ) == MOON_NEW );
    CHECK( get_moon_phase( zero + 31_days ) == MOON_NEW );
    CHECK( get_moon_phase( zero + 32_days ) == MOON_WAXING_CRESCENT );
}

// To prevent the light level from abruptly changing in the middle of the night, the moon's phase
// change occurs at noon (rather than midnight) on the appropriate day during the lunar cycle.
//
TEST_CASE( "moon_phase_changes_at_noon", "[calendar][moon][phase][change]" )
{
    calendar::set_season_length( 91 );
    REQUIRE( calendar::season_from_default_ratio() == Approx( 1.0f ) );

    const time_point zero = calendar::turn_zero;

    CHECK( get_moon_phase( zero + 1_days + 11_hours ) == MOON_NEW );
    CHECK( get_moon_phase( zero + 1_days + 13_hours ) == MOON_WAXING_CRESCENT );

    CHECK( get_moon_phase( zero + 5_days + 11_hours ) == MOON_WAXING_CRESCENT );
    CHECK( get_moon_phase( zero + 5_days + 13_hours ) == MOON_HALF_MOON_WAXING );

    CHECK( get_moon_phase( zero + 9_days + 11_hours ) == MOON_HALF_MOON_WAXING );
    CHECK( get_moon_phase( zero + 9_days + 13_hours ) == MOON_WAXING_GIBBOUS );

    CHECK( get_moon_phase( zero + 12_days + 11_hours ) == MOON_WAXING_GIBBOUS );
    CHECK( get_moon_phase( zero + 12_days + 13_hours ) == MOON_FULL );

    CHECK( get_moon_phase( zero + 16_days + 11_hours ) == MOON_FULL );
    CHECK( get_moon_phase( zero + 16_days + 13_hours ) == MOON_WANING_GIBBOUS );

    CHECK( get_moon_phase( zero + 20_days + 11_hours ) == MOON_WANING_GIBBOUS );
    CHECK( get_moon_phase( zero + 20_days + 13_hours ) == MOON_HALF_MOON_WANING );

    CHECK( get_moon_phase( zero + 23_days + 11_hours ) == MOON_HALF_MOON_WANING );
    CHECK( get_moon_phase( zero + 23_days + 13_hours ) == MOON_WANING_CRESCENT );

    CHECK( get_moon_phase( zero + 27_days + 11_hours ) == MOON_WANING_CRESCENT );
    CHECK( get_moon_phase( zero + 27_days + 13_hours ) == MOON_NEW );

    CHECK( get_moon_phase( zero + 31_days + 11_hours ) == MOON_NEW );
    CHECK( get_moon_phase( zero + 31_days + 13_hours ) == MOON_WAXING_CRESCENT );
}

// ---------
// Moonlight
// ---------

// At dawn, light transitions from moonlight to sunlight level.
// At dusk, light transitions from sunlight to moonlight level.
TEST_CASE( "moonlight_at_dawn_and_dusk", "[calendar][moon][moonlight][dawn][dusk]" )
{
    calendar::set_season_length( 91 );
    REQUIRE( calendar::season_from_default_ratio() == Approx( 1.0f ) );

    WHEN( "moon is new" ) {
        time_point new_phase = calendar::turn_zero;
        // Midnight of new moon
        time_point new_midnight = new_phase - time_past_midnight( new_phase );
        time_point new_sunrise = sunrise( new_midnight );
        time_point new_sunset = sunset( new_midnight );
        time_point new_noon = new_midnight + 12_hours;

        // Daylight level should be ~100 at first new moon
        float daylight_level = sun_moon_light_at( new_noon );
        CHECK( daylight_level == Approx( 110 ).margin( 10 ) );
        float moonlight_level = 1.0f;

        THEN( "at night, light is only moonlight" ) {
            CHECK( sun_moon_light_at( new_sunset + 2_hours ) == moonlight_level );
            CHECK( sun_moon_light_at( new_midnight ) == moonlight_level );
            CHECK( sun_moon_light_at( new_sunrise - 2_hours ) == moonlight_level );
        }
        THEN( "at dawn, light increases from moonlight to daylight" ) {
            CHECK( sun_moon_light_at( new_sunrise ) > sun_moon_light_at( new_sunrise - 2_hours ) );
            CHECK( sun_moon_light_at( new_sunrise + 1_hours ) ==
                   sun_light_at( new_sunrise + 1_hours ) + moonlight_level );
        }
    }

    WHEN( "moon is full" ) {
        time_point full_phase = calendar::turn_zero + lunar_month() / 2;
        // Midnight of full moon
        time_point full_midnight = full_phase - time_past_midnight( full_phase );
        time_point full_sunrise = sunrise( full_midnight );
        time_point full_sunset = sunset( full_midnight );
        time_point full_noon = full_midnight + 12_hours;

        // Daylight level is higher, later in the season (~104 at first full moon)
        float daylight_level = sun_moon_light_at( full_noon );
        CHECK( daylight_level == Approx( 120 ).margin( 10 ) );
        float moonlight_level = 7.0f;

        THEN( "at night, light is only moonlight" ) {
            CHECK( sun_moon_light_at( full_sunset + 2_hours ) == moonlight_level );
            CHECK( sun_moon_light_at( full_midnight ) == moonlight_level );
            CHECK( sun_moon_light_at( full_sunrise - 2_hours ) == moonlight_level );
        }
        THEN( "at dawn, light increases from moonlight to daylight" ) {
            CHECK( sun_moon_light_at( full_sunrise ) >
                   sun_moon_light_at( full_sunrise - 2_hours ) );
            CHECK( sun_moon_light_at( full_sunrise + 1_hours ) ==
                   sun_light_at( full_sunrise + 1_hours ) + moonlight_level );
        }
    }
}

// Return the midnight of a day in the lunar month, scaled as [0=new, 0.5=half, 1=full, 2=new]
static time_point lunar_month_night( const float phase_scale )
{
    REQUIRE( 0.0f <= phase_scale );
    REQUIRE( phase_scale <= 2.0f );

    // Adjust clock such that 1.0 is the full moon, and 2.0 is the full lunar month
    time_point this_phase = calendar::turn_zero + phase_scale * lunar_month() / 2;

    // Roll back or forward to the nearest midnight
    time_point this_night;
    time_duration past_midnight = time_past_midnight( this_phase );

    // Use this midnight if it's still morning (a.m.), otherwise tomorrow midnight
    if( past_midnight < 12_hours ) {
        this_night = this_phase - past_midnight;
    } else {
        this_night = this_phase - past_midnight + 24_hours;
    }

    return this_night;
}

// Return moonlight with moon phase on a scale of (0=new, 0.5=half, 1.0=full, .. 2.0=new again)
static float phase_moonlight( const float phase_scale, const moon_phase expect_phase_enum )
{
    // Get midnight on the desired day of the lunary month
    time_point this_night = lunar_month_night( phase_scale );

    // Ensure we are in the expected phase
    CAPTURE( phase_scale );
    CHECK( io::enum_to_string( get_moon_phase( this_night ) ) == io::enum_to_string(
               expect_phase_enum ) );

    // Finally, get the amount of moonlight
    return sun_moon_light_at( this_night );
}

// Moonlight level varies with moon phase, from 1.0 at new moon to 10.0 at full moon.
TEST_CASE( "moonlight_for_each_phase", "[calendar][moon][phase][moonlight]" )
{
    calendar::set_season_length( 91 );
    REQUIRE( calendar::season_from_default_ratio() == Approx( 1.0f ) );

    // At the start of each phase, moonlight is 1.0 + (2.25 per quarter)
    SECTION( "moonlight increases as moon goes from new to full" ) {
        CHECK( 1.00f == phase_moonlight( 0.0f, MOON_NEW ) );
        CHECK( 2.5f == phase_moonlight( 0.25f, MOON_WAXING_CRESCENT ) );
        CHECK( 4.f == phase_moonlight( 0.5f, MOON_HALF_MOON_WAXING ) );
        CHECK( 5.5f == phase_moonlight( 0.75f, MOON_WAXING_GIBBOUS ) );
        CHECK( 7.0f == phase_moonlight( 1.0f, MOON_FULL ) );
        CHECK( 5.5f == phase_moonlight( 1.25f, MOON_WANING_GIBBOUS ) );
        CHECK( 4.f == phase_moonlight( 1.5f, MOON_HALF_MOON_WANING ) );
        CHECK( 2.5f == phase_moonlight( 1.75f, MOON_WANING_CRESCENT ) );
    }

    SECTION( "moonlight is constant during each phase" ) {
        // FIXME: Moonlight should follow a curve, rather than discrete plateaus based on phase
        // http://adsabs.harvard.edu/full/1966JRASC..60..221E
        CHECK( 1.0f == phase_moonlight( 0.0f, MOON_NEW ) );
        CHECK( 1.0f == phase_moonlight( 0.10f, MOON_NEW ) );
        // FIXME: Make moonlight gradually transition from one phase to the next
        // ex., from NEW to WAXING_CRESCENT should smoothly go from 1.0 to 3.25
        // instead of jumping suddenly in the middle.
        CHECK( 2.5f == phase_moonlight( 0.11f, MOON_WAXING_CRESCENT ) );
        CHECK( 2.5f == phase_moonlight( 0.20f, MOON_WAXING_CRESCENT ) );
        CHECK( 2.5f == phase_moonlight( 0.30f, MOON_WAXING_CRESCENT ) );
    }
}

