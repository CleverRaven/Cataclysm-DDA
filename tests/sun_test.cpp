#include "calendar.h"

#include "catch/catch.hpp"

// SUN TESTS

static const time_point midday = calendar::turn_zero + 12_hours;
static const time_point midnight = calendar::turn_zero + 0_hours;

const time_point today_sunrise = sunrise( midnight );
const time_point today_sunset = sunset( midnight );

// is_night, is_sunrise_now, is_sunset_now
TEST_CASE( "daily solar cycle", "[sun][cycle]" )
{
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
    CHECK( sunlight( midnight ) == 1.0f );
    CHECK( sunlight( today_sunrise ) == 1.0f );
    CHECK( sunlight( today_sunrise + 15_minutes ) == 25.75f );
    CHECK( sunlight( today_sunrise + 30_minutes ) == 50.50f );
    CHECK( sunlight( today_sunrise + 45_minutes ) == 75.25f );
    CHECK( sunlight( midday ) == default_daylight_level() ); // 100.0
    CHECK( sunlight( today_sunset ) == default_daylight_level() ); // 100.0
    CHECK( sunlight( today_sunset + 15_minutes ) == 75.25f );
    CHECK( sunlight( today_sunset + 30_minutes ) == 50.50f );
    CHECK( sunlight( today_sunset + 45_minutes ) == 25.75f );
}

TEST_CASE( "sunrise", "[sun][sunrise]" )
{
    const time_point spring = calendar::turn_zero;
    const time_point summer = spring + 91_days;
    const time_point autumn = summer + 91_days;
    const time_point winter = autumn + 91_days;

    CHECK( "Year 1, Spring, day 1 6:00:00 AM" == to_string( sunrise( spring ) ) );
    CHECK( "Year 1, Spring, day 31 5:40:00 AM" == to_string( sunrise( spring + 30_days ) ) );
    CHECK( "Year 1, Spring, day 61 5:20:00 AM" == to_string( sunrise( spring + 60_days ) ) );
    CHECK( "Year 1, Spring, day 91 5:00:00 AM" == to_string( sunrise( spring + 90_days ) ) );

    CHECK( "Year 1, Summer, day 1 5:00:00 AM" == to_string( sunrise( summer ) ) );
    CHECK( "Year 1, Summer, day 31 5:19:00 AM" == to_string( sunrise( summer + 30_days ) ) );
    CHECK( "Year 1, Summer, day 61 5:39:00 AM" == to_string( sunrise( summer + 60_days ) ) );
    CHECK( "Year 1, Summer, day 91 5:59:00 AM" == to_string( sunrise( summer + 90_days ) ) );

    CHECK( "Year 1, Autumn, day 1 6:00:00 AM" == to_string( sunrise( autumn ) ) );
    CHECK( "Year 1, Autumn, day 31 6:19:00 AM" == to_string( sunrise( autumn + 30_days ) ) );
    CHECK( "Year 1, Autumn, day 61 6:39:00 AM" == to_string( sunrise( autumn + 60_days ) ) );
    CHECK( "Year 1, Autumn, day 91 6:59:00 AM" == to_string( sunrise( autumn + 90_days ) ) );

    CHECK( "Year 1, Winter, day 1 7:00:00 AM" == to_string( sunrise( winter ) ) );
    CHECK( "Year 1, Winter, day 31 6:40:00 AM" == to_string( sunrise( winter + 30_days ) ) );
    CHECK( "Year 1, Winter, day 61 6:20:00 AM" == to_string( sunrise( winter + 60_days ) ) );
    CHECK( "Year 1, Winter, day 91 6:00:00 AM" == to_string( sunrise( winter + 90_days ) ) );
}

TEST_CASE( "sunset", "[sun][sunset]" )
{
    const time_point spring = calendar::turn_zero;
    const time_point summer = spring + 91_days;
    const time_point autumn = summer + 91_days;
    const time_point winter = autumn + 91_days;

    CHECK( "Year 1, Spring, day 1 7:00:00 PM" == to_string( sunset( spring ) ) );
    CHECK( "Year 1, Spring, day 31 7:39:00 PM" == to_string( sunset( spring + 30_days ) ) );
    CHECK( "Year 1, Spring, day 61 8:19:00 PM" == to_string( sunset( spring + 60_days ) ) );
    CHECK( "Year 1, Spring, day 91 8:58:00 PM" == to_string( sunset( spring + 90_days ) ) );

    CHECK( "Year 1, Summer, day 1 9:00:00 PM" == to_string( sunset( summer ) ) );
    CHECK( "Year 1, Summer, day 31 8:20:00 PM" == to_string( sunset( summer + 30_days ) ) );
    CHECK( "Year 1, Summer, day 61 7:40:00 PM" == to_string( sunset( summer + 60_days ) ) );
    CHECK( "Year 1, Summer, day 91 7:01:00 PM" == to_string( sunset( summer + 90_days ) ) );

    CHECK( "Year 1, Autumn, day 1 7:00:00 PM" == to_string( sunset( autumn ) ) );
    CHECK( "Year 1, Autumn, day 31 6:20:00 PM" == to_string( sunset( autumn + 30_days ) ) );
    CHECK( "Year 1, Autumn, day 61 5:40:00 PM" == to_string( sunset( autumn + 60_days ) ) );
    CHECK( "Year 1, Autumn, day 91 5:01:00 PM" == to_string( sunset( autumn + 90_days ) ) );

    CHECK( "Year 1, Winter, day 1 5:00:00 PM" == to_string( sunset( winter ) ) );
    CHECK( "Year 1, Winter, day 31 5:39:00 PM" == to_string( sunset( winter + 30_days ) ) );
    CHECK( "Year 1, Winter, day 61 6:19:00 PM" == to_string( sunset( winter + 60_days ) ) );
    CHECK( "Year 1, Winter, day 91 6:58:00 PM" == to_string( sunset( winter + 90_days ) ) );
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
    const time_point full_moon = calendar::turn_zero + calendar::season_length() / 6;
    REQUIRE( get_moon_phase( full_moon ) == MOON_FULL );
}

