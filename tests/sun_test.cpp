#include "calendar.h"

#include "catch/catch.hpp"

// SUN TESTS

// is_night, is_dawn, is_dusk
TEST_CASE( "daily solar cycle", "[sun][night][dawn][day][dusk]" )
{
    // Use sunrise/sunset on the first day (spring equinox)
    static const time_point midnight = calendar::turn_zero;
    static const time_point today_sunrise = sunrise( midnight );
    static const time_point today_sunset = sunset( midnight );

    REQUIRE( "Year 1, Spring, day 1 6:00:00 AM" == to_string( today_sunrise ) );
    REQUIRE( "Year 1, Spring, day 1 7:00:00 PM" == to_string( today_sunset ) );

    // 00:00 is_night
    //   :   is_night
    // 06:00 is_night && is_dawn (sunrise time)
    //   :   is_dawn (sunrise + twilight )
    // 07:00 is_dawn && is_day
    //   :   is_day
    // 19:00 is_day && is_dusk (sunset time)
    //   :   is_dusk (sunset + twilight )
    // 20:00 is_dusk && is_night
    //   :   is_night
    // 23:59 is_night

    CHECK( is_night( midnight ) );
    CHECK( is_night( midnight + 1_seconds ) );
    CHECK( is_night( midnight + 2_hours ) );
    CHECK( is_night( midnight + 3_hours ) );
    CHECK( is_night( midnight + 4_hours ) );

    // The point of sunrise is both "night" and "dawn"
    CHECK( is_night( today_sunrise ) );
    CHECK( is_dawn( today_sunrise ) );

    // Dawn
    CHECK( is_dawn( today_sunrise + 1_seconds ) );
    CHECK( is_dawn( today_sunrise + 30_minutes ) );
    CHECK( is_dawn( today_sunrise + 59_minutes ) );

    // The endpoint of dawn is both "dawn" and "day"
    CHECK( is_dawn( today_sunrise + 1_hours ) );
    //CHECK( is_day( today_sunrise + 1_hours ) );

    // FIXME: No function to tell when it is daytime

    // The beginning of sunset is both "day" and "dusk"
    //CHECK( is_day( today_sunset ) );
    CHECK( is_dusk( today_sunset ) );

    // Dusk
    CHECK( is_dusk( today_sunset + 1_seconds ) );
    CHECK( is_dusk( today_sunset + 30_minutes ) );
    CHECK( is_dusk( today_sunset + 59_minutes ) );

    // The point when dusk ends is both "dusk" and "night"
    CHECK( is_dusk( today_sunset + 1_hours ) );
    CHECK( is_night( today_sunset + 1_hours ) );

    // Night again
    CHECK( is_night( today_sunset + 1_hours + 1_seconds ) );
    CHECK( is_night( today_sunset + 2_hours ) );
    CHECK( is_night( today_sunset + 3_hours ) );
    CHECK( is_night( today_sunset + 4_hours ) );
}

// current_daylight_level returns seasonally-adjusted maximum daylight level
TEST_CASE( "current daylight level", "[sun][daylight]" )
{
    static const time_duration one_season = calendar::season_length();
    static const time_point spring = calendar::turn_zero;
    static const time_point summer = spring + one_season;
    static const time_point autumn = summer + one_season;
    static const time_point winter = autumn + one_season;

    // For ~Boston: solstices are +/- 25% sunlight intensity from equinoxes
    CHECK( 100.0f == current_daylight_level( spring ) );
    CHECK( 125.0f == current_daylight_level( summer ) );
    CHECK( 100.0f == current_daylight_level( autumn ) );
    CHECK( 75.0f == current_daylight_level( winter ) );
}

// The calendar `sunlight` function returns light level for both sun and moon.
TEST_CASE( "sunlight and moonlight", "[sun][sunlight][moonlight]" )
{
    // Use sunrise/sunset on the first day (spring equinox)
    static const time_point midnight = calendar::turn_zero;
    static const time_point today_sunrise = sunrise( midnight );
    static const time_point today_sunset = sunset( midnight );

    // Tests assume 100.0f maximum
    REQUIRE( default_daylight_level() == 100.0f );

    SECTION( "sunlight" ) {
        // Before dawn
        CHECK( sunlight( midnight ) == 1.0f );
        CHECK( sunlight( today_sunrise ) == 1.0f );
        // Dawn
        CHECK( sunlight( today_sunrise + 1_seconds ) == Approx( 1.0275f ) );
        CHECK( sunlight( today_sunrise + 1_minutes ) == Approx( 2.65f ) );
        CHECK( sunlight( today_sunrise + 15_minutes ) == 25.75f );
        CHECK( sunlight( today_sunrise + 15_minutes ) == 25.75f );
        CHECK( sunlight( today_sunrise + 30_minutes ) == 50.50f );
        CHECK( sunlight( today_sunrise + 45_minutes ) == 75.25f );
        // 1 second before full daylight
        CHECK( sunlight( today_sunrise + 1_hours - 1_seconds ) == Approx( 99.9725f ) );
        CHECK( sunlight( today_sunrise + 1_hours ) == 100.0f );
        // End of dawn, full light all day
        CHECK( sunlight( today_sunrise + 2_hours ) == 100.0f );
        CHECK( sunlight( today_sunrise + 3_hours ) == 100.0f );
        // Noon
        CHECK( sunlight( midnight + 12_hours ) == 100.0f );
        CHECK( sunlight( midnight + 13_hours ) == 100.0f );
        CHECK( sunlight( midnight + 14_hours ) == 100.0f );
        // Dusk begins
        CHECK( sunlight( today_sunset ) == 100.0f );
        // 1 second after dusk begins
        CHECK( sunlight( today_sunset + 1_seconds ) == Approx( 99.9725f ) );
        CHECK( sunlight( today_sunset + 15_minutes ) == 75.25f );
        CHECK( sunlight( today_sunset + 30_minutes ) == 50.50f );
        CHECK( sunlight( today_sunset + 45_minutes ) == 25.75f );
        // 1 second before full night
        CHECK( sunlight( today_sunset + 1_hours - 1_seconds ) == Approx( 1.0275f ) );
        CHECK( sunlight( today_sunset + 1_hours ) == 1.0f );
        // After dusk
        CHECK( sunlight( today_sunset + 2_hours ) == 1.0f );
        CHECK( sunlight( today_sunset + 3_hours ) == 1.0f );
    }

    // This moonlight test is intentionally simple, only checking new moon (minimal light) and full
    // moon (maximum moonlight). More detailed tests of moon phase and light should be expressed in
    // `moon_test.cpp`. Including here simply to check that `sunlight` also calculates moonlight.
    SECTION( "moonlight" ) {
        static const time_duration phase_time = calendar::season_length() / 6;
        static const time_point new_moon = calendar::turn_zero;
        static const time_point full_moon = new_moon + phase_time;

        WHEN( "the moon is new" ) {
            REQUIRE( get_moon_phase( new_moon ) == MOON_NEW );
            THEN( "moonlight is 1.0" ) {
                CHECK( sunlight( new_moon ) == 1.0f );
            }
        }

        WHEN( "the moon is full" ) {
            REQUIRE( get_moon_phase( full_moon ) == MOON_FULL );
            THEN( "moonlight is 10.0" ) {
                CHECK( sunlight( full_moon ) == 10.0f );
            }
        }
    }
}

// The times of sunrise and sunset vary throughout the year. For simplicity, equinoxes occur on the
// first day of spring and autumn, and solstices occur on the first day of summer and winter.
TEST_CASE( "sunrise and sunset", "[sun][sunrise][sunset][equinox][solstice]" )
{
    // Due to the "NN_days" math below, this test requires a default 91-day season length
    REQUIRE( calendar::season_from_default_ratio() == Approx( 1.0f ) );

    static const time_duration one_season = calendar::season_length();
    static const time_point spring = calendar::turn_zero;
    static const time_point summer = spring + one_season;
    static const time_point autumn = summer + one_season;
    static const time_point winter = autumn + one_season;

    // The expected sunrise/sunset times depend on internal values in `calendar.cpp` including:
    // - sunrise_winter, sunrise_summer, sunrise_equinox
    // - sunset_winter, sunset_summer, sunset_equinox
    // These being constants based on the default game setting in New England, planet Earth.
    // Were these to become variable, the tests would need to adapt.

    SECTION( "spring equinox is day 1 of spring" ) {
        // 11 hours of daylight
        CHECK( "Year 1, Spring, day 1 6:00:00 AM" == to_string( sunrise( spring ) ) );
        CHECK( "Year 1, Spring, day 1 7:00:00 PM" == to_string( sunset( spring ) ) );

        THEN( "sunrise gets earlier" ) {
            CHECK( "6:00:00 AM" == to_string_time_of_day( sunrise( spring ) ) );
            CHECK( "5:40:00 AM" == to_string_time_of_day( sunrise( spring + 30_days ) ) );
            CHECK( "5:20:00 AM" == to_string_time_of_day( sunrise( spring + 60_days ) ) );
            CHECK( "5:00:00 AM" == to_string_time_of_day( sunrise( spring + 90_days ) ) );
        }
        THEN( "sunset gets later" ) {
            CHECK( "7:00:00 PM" == to_string_time_of_day( sunset( spring ) ) );
            CHECK( "7:39:00 PM" == to_string_time_of_day( sunset( spring + 30_days ) ) );
            CHECK( "8:19:00 PM" == to_string_time_of_day( sunset( spring + 60_days ) ) );
            CHECK( "8:58:00 PM" == to_string_time_of_day( sunset( spring + 90_days ) ) );
        }
    }

    SECTION( "summer solstice is day 1 of summer" ) {
        // 14 hours of daylight
        CHECK( "Year 1, Summer, day 1 5:00:00 AM" == to_string( sunrise( summer ) ) );
        CHECK( "Year 1, Summer, day 1 9:00:00 PM" == to_string( sunset( summer ) ) );

        THEN( "sunrise gets later" ) {
            CHECK( "5:00:00 AM" == to_string_time_of_day( sunrise( summer ) ) );
            CHECK( "5:19:00 AM" == to_string_time_of_day( sunrise( summer + 30_days ) ) );
            CHECK( "5:39:00 AM" == to_string_time_of_day( sunrise( summer + 60_days ) ) );
            CHECK( "5:59:00 AM" == to_string_time_of_day( sunrise( summer + 90_days ) ) );
        }
        THEN( "sunset gets earlier" ) {
            CHECK( "9:00:00 PM" == to_string_time_of_day( sunset( summer ) ) );
            CHECK( "8:20:00 PM" == to_string_time_of_day( sunset( summer + 30_days ) ) );
            CHECK( "7:40:00 PM" == to_string_time_of_day( sunset( summer + 60_days ) ) );
            CHECK( "7:01:00 PM" == to_string_time_of_day( sunset( summer + 90_days ) ) );
        }
    }

    SECTION( "autumn equinox is day 1 of autumn" ) {
        // 11 hours of daylight
        CHECK( "Year 1, Autumn, day 1 6:00:00 AM" == to_string( sunrise( autumn ) ) );
        CHECK( "Year 1, Autumn, day 1 7:00:00 PM" == to_string( sunset( autumn ) ) );

        THEN( "sunrise gets later" ) {
            CHECK( "6:00:00 AM" == to_string_time_of_day( sunrise( autumn ) ) );
            CHECK( "6:19:00 AM" == to_string_time_of_day( sunrise( autumn + 30_days ) ) );
            CHECK( "6:39:00 AM" == to_string_time_of_day( sunrise( autumn + 60_days ) ) );
            CHECK( "6:59:00 AM" == to_string_time_of_day( sunrise( autumn + 90_days ) ) );
        }
        THEN( "sunset gets earlier" ) {
            CHECK( "7:00:00 PM" == to_string_time_of_day( sunset( autumn ) ) );
            CHECK( "6:20:00 PM" == to_string_time_of_day( sunset( autumn + 30_days ) ) );
            CHECK( "5:40:00 PM" == to_string_time_of_day( sunset( autumn + 60_days ) ) );
            CHECK( "5:01:00 PM" == to_string_time_of_day( sunset( autumn + 90_days ) ) );
        }
    }

    SECTION( "winter solstice is day 1 of winter" ) {
        // 10 hours of daylight
        CHECK( "Year 1, Winter, day 1 7:00:00 AM" == to_string( sunrise( winter ) ) );
        CHECK( "Year 1, Winter, day 1 5:00:00 PM" == to_string( sunset( winter ) ) );

        THEN( "sunrise gets earlier" ) {
            CHECK( "7:00:00 AM" == to_string_time_of_day( sunrise( winter ) ) );
            CHECK( "6:40:00 AM" == to_string_time_of_day( sunrise( winter + 30_days ) ) );
            CHECK( "6:20:00 AM" == to_string_time_of_day( sunrise( winter + 60_days ) ) );
            CHECK( "6:00:00 AM" == to_string_time_of_day( sunrise( winter + 90_days ) ) );
        }
        THEN( "sunset gets later" ) {
            CHECK( "5:00:00 PM" == to_string_time_of_day( sunset( winter ) ) );
            CHECK( "5:39:00 PM" == to_string_time_of_day( sunset( winter + 30_days ) ) );
            CHECK( "6:19:00 PM" == to_string_time_of_day( sunset( winter + 60_days ) ) );
            CHECK( "6:58:00 PM" == to_string_time_of_day( sunset( winter + 90_days ) ) );
        }
    }
}

