#include "calendar.h"

#include "catch/catch.hpp"

// SUN TESTS

static const time_point midday = calendar::turn_zero + 12_hours;
static const time_point midnight = calendar::turn_zero + 0_hours;

// FIXME - rework these

// is_night, is_sunrise_now, is_sunset_now
TEST_CASE( "daily solar cycle", "[sun][cycle]" )
{
    // TODO: Generalize based on sunrise/sunset

    // Use sunrise/sunset on the first day (spring equinox)
    const time_point today_sunrise = sunrise( calendar::turn_zero );
    const time_point today_sunset = sunset( calendar::turn_zero );

    // INFO( "Sunrise today: " + to_string( today_sunrise ) ); // 6:00 AM
    // INFO( "Sunset today: " + to_string( today_sunset ) ); // 7:00 PM

    // Night
    CHECK( is_night( midnight ) );
    CHECK( is_night( midnight + 1_hours ) );
    CHECK( is_night( midnight + 2_hours ) );
    CHECK( is_night( midnight + 3_hours ) );
    CHECK( is_night( midnight + 4_hours ) );
    CHECK( is_night( midnight + 5_hours ) );

    // FIXME: This time is neither night nor sunrise
    CHECK_FALSE( is_night( today_sunrise ) );
    CHECK_FALSE( is_sunrise_now( today_sunrise ) );

    // Dawn
    CHECK( is_sunrise_now( today_sunrise + 1_seconds ) );
    CHECK( is_sunrise_now( today_sunrise + 30_minutes ) );
    CHECK( is_sunrise_now( today_sunrise + 59_minutes ) );
    CHECK_FALSE( is_sunrise_now( today_sunrise + 1_hours ) );

    // FIXME: No function to tell when it is daytime
    //CHECK( is_day( midday ) );

    // FIXME: Sunset does not begin at sunset
    CHECK_FALSE( is_sunset_now( today_sunset ) );

    // Dusk
    CHECK( is_sunset_now( today_sunset + 1_seconds ) );
    CHECK( is_sunset_now( today_sunset + 30_minutes ) );
    CHECK( is_sunset_now( today_sunset + 59_minutes ) );

    // FIXME: This time is neither dusk nor night
    CHECK_FALSE( is_sunset_now( today_sunset + 1_hours ) );
    CHECK_FALSE( is_night( today_sunset + 1_hours ) );

    // Night again
    CHECK( is_night( today_sunset + 1_hours + 1_seconds ) );
    CHECK( is_night( today_sunset + 2_hours ) );
    CHECK( is_night( today_sunset + 3_hours ) );
    CHECK( is_night( today_sunset + 4_hours ) );
}

TEST_CASE( "sunlight", "[sun]" )
{
    const time_point today_sunrise = sunrise( calendar::turn_zero );
    const time_point today_sunset = sunset( calendar::turn_zero );

    CHECK( sunlight( midnight ) == 1.0f );
    CHECK( sunlight( today_sunrise ) == 1.0f );
    // Dawn
    CHECK( sunlight( today_sunrise + 15_minutes ) == 25.75f );
    CHECK( sunlight( today_sunrise + 30_minutes ) == 50.50f );
    CHECK( sunlight( today_sunrise + 45_minutes ) == 75.25f );
    // Midday
    CHECK( sunlight( midday ) == default_daylight_level() ); // 100.0
    CHECK( sunlight( today_sunset ) == default_daylight_level() ); // 100.0
    // Dusk
    CHECK( sunlight( today_sunset + 15_minutes ) == 75.25f );
    CHECK( sunlight( today_sunset + 30_minutes ) == 50.50f );
    CHECK( sunlight( today_sunset + 45_minutes ) == 25.75f );
}

TEST_CASE( "sunrise", "[sun][sunrise][sunset]" )
{
    static const time_duration one_season = calendar::season_length();
    static const time_point spring = calendar::turn_zero;
    static const time_point summer = spring + one_season;
    static const time_point autumn = summer + one_season;
    static const time_point winter = autumn + one_season;

    SECTION( "spring equinox is day 1 of spring" ) {
        // - sunrise_equinox = (average of summer and winter sunrises)
        // - sunset_equinox = (average of summer and winter sunsets)
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
        // Summer solstice:
        // - sunrise_summer = 5
        // - sunser_summer = 21
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
        // - sunrise_equinox = (average of summer and winter sunrises)
        // - sunset_equinox = (average of summer and winter sunsets)
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
        // Winter solstice:
        // - sunrise_winter = 7
        // - sunser_winter = 17
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

TEST_CASE( "current daylight level", "[sun][daylight]" )
{
    // TODO
    // based on season_of_year
    // SPRING
    // SUMMER
    // AUTUMN
    // WINTER
}

TEST_CASE( "full moon", "[moon]" )
{
    /*
    const time_point full_moon = calendar::turn_zero + calendar::season_length() / 6;
    REQUIRE( get_moon_phase( full_moon ) == MOON_FULL );
    */
}

