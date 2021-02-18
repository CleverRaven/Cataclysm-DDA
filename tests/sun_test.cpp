#include "catch/catch.hpp"
#include "calendar.h" // IWYU pragma: associated
#include "units.h"
#include "options.h"
#include "options_helpers.h"

#include <string>


TEST_CASE( "daily solar cycle", "[sun][night][dawn][day][dusk]" )
{

    // The 24-hour solar cycle is divided into four parts, as returned by four calendar.cpp functions:
    //
    // is_night : From -12 degrees to -12 degrees
    // is_dawn  : From -18 degrees to 1 degrees
    // is_day   : From 0 degrees to 0 degrees
    // is_dusk  : From 1 degrees to -18 degrees
    //
    // These are inclusive at their endpoints; in other words, each overlaps with the next:
    //
    // This test covers is_night, is_dawn, is_day, is_dusk, and their behavior in relation to time of day.

    // Use sunrise/sunset on the first day (spring equinox)
    static const time_point midnight = calendar::turn_zero;
    static const time_point noon = midnight + 12_hours;
    static const time_point today_sunrise = sunrise( midnight );
    static const time_point today_sunset = sunset( midnight );

    static const time_point dawn_start = sun_at_angle( units::from_degrees( -18 ), midnight, false );
    static const time_point dusk_end = sun_at_angle( units::from_degrees( -18 ), midnight, true );

    SECTION( "dawn and sunset time" ) {
        CHECK( "Year 1, Spring, day 1 7:29:58 AM" == to_string( today_sunrise ) );
        CHECK( "Year 1, Spring, day 1 4:30:01 PM" == to_string( today_sunset ) );
    }

    SECTION( "Night" ) {
        // First, at the risk of stating the obvious
        CHECK( is_night( midnight ) );
        // And while we're at it
        CHECK_FALSE( is_day( midnight ) );
        CHECK_FALSE( is_dawn( midnight ) );
        CHECK_FALSE( is_dusk( midnight ) );

        // Yep, still dark
        CHECK( is_night( midnight + 1_seconds ) );
        CHECK( is_night( midnight + 2_hours ) );
        CHECK( is_night( midnight + 3_hours ) );
        CHECK( is_night( midnight + 4_hours ) );

        // At -18 degrees it is both dawn and night
        CHECK( is_night( dawn_start ) );
        CHECK( is_dawn( dawn_start ) );
    }

    SECTION( "Dawn" ) {
        CHECK_FALSE( is_night( today_sunrise ) );
        CHECK( is_dawn( today_sunrise - 1_seconds ) );
        CHECK( is_dawn( today_sunrise - 30_minutes ) );

        // Dawn stops at 1 degrees
        CHECK_FALSE( is_dawn( today_sunrise + 7_minutes ) );
    }

    SECTION( "Day" ) {
        // Due to roundings the day may start few seconds later than expected
        CHECK( is_day( today_sunrise + 2_seconds ) );
        CHECK( is_day( today_sunrise + 2_hours ) );
        // Second breakfast
        CHECK( is_day( today_sunrise + 3_hours ) );
        CHECK( is_day( today_sunrise + 4_hours ) );
        // Luncheon
        CHECK( is_day( noon - 3_hours ) );
        CHECK( is_day( noon - 2_hours ) );
        // Elevenses
        CHECK( is_day( noon - 1_hours ) );
        // Noon
        CHECK( is_day( noon ) );
        CHECK_FALSE( is_dawn( noon ) );
        CHECK_FALSE( is_dusk( noon ) );
        CHECK_FALSE( is_night( noon ) );
        // Afternoon tea
        CHECK( is_day( noon + 1_hours ) );
        CHECK( is_day( noon + 2_hours ) );
        // Dinner
        CHECK( is_day( noon + 3_hours ) );
        CHECK( is_day( today_sunset - 2_hours ) );
        // Supper
        CHECK( is_day( today_sunset - 1_hours ) );
        CHECK( is_day( today_sunset - 1_seconds ) );
    }

    SECTION( "Dusk" ) {
        // Sun setting down is both "day" and "dusk"
        CHECK( is_day( today_sunset ) );
        CHECK( is_dusk( today_sunset ) );

        // Dusk
        CHECK_FALSE( is_day( today_sunset + 1_seconds ) );
        CHECK( is_dusk( today_sunset + 1_seconds ) );
        CHECK( is_dusk( today_sunset + 30_minutes ) );
        CHECK( is_dusk( today_sunset + 1_hours - 1_seconds ) );
    }

    SECTION( "Night again" ) {
        CHECK( is_dusk( dusk_end ) );
        CHECK( is_night( dusk_end ) );

        CHECK( is_night( dusk_end + 2_hours ) );
        CHECK( is_night( dusk_end + 3_hours ) );
        CHECK( is_night( dusk_end + 4_hours ) );
    }
}

// The calendar `sunlight` function returns light level for both sun and moon.
TEST_CASE( "sunlight and moonlight", "[sun][sunlight][moonlight]" )
{
    // Use sunrise/sunset on the first day (spring equinox)
    static const time_point midnight = calendar::turn_zero;
    static const time_point today_sunrise = sunrise( midnight );
    static const time_point today_sunset = sunset( midnight );

    // Expected numbers below assume 100.0f maximum daylight level
    // (maximum daylight is different at other times of year - see [daylight] tests)
    REQUIRE( 100.0f == default_daylight_level() );

    SECTION( "sunlight" ) {
        // Before dawn
        CHECK( 1.0f == sunlight( midnight ) );
        CHECK( 1.0f == sunlight( today_sunrise ) );
        // Dawn
        CHECK( 1.0275f == sunlight( today_sunrise + 1_seconds ) );
        CHECK( 2.65f == sunlight( today_sunrise + 1_minutes ) );
        CHECK( 25.75f == sunlight( today_sunrise + 15_minutes ) );
        CHECK( 50.50f == sunlight( today_sunrise + 30_minutes ) );
        CHECK( 75.25f == sunlight( today_sunrise + 45_minutes ) );
        // 1 second before full daylight
        CHECK( 99.9725f == sunlight( today_sunrise + 1_hours - 1_seconds ) );
        CHECK( 100.0f == sunlight( today_sunrise + 1_hours ) );
        // End of dawn, full light all day
        CHECK( 100.0f == sunlight( today_sunrise + 2_hours ) );
        CHECK( 100.0f == sunlight( today_sunrise + 3_hours ) );
        // Noon
        CHECK( 100.0f == sunlight( midnight + 12_hours ) );
        CHECK( 100.0f == sunlight( midnight + 13_hours ) );
        CHECK( 100.0f == sunlight( midnight + 14_hours ) );
        // Dusk begins
        CHECK( 100.0f == sunlight( today_sunset ) );
        // 1 second after dusk begins
        CHECK( 99.9725f == sunlight( today_sunset + 1_seconds ) );
        CHECK( 75.25f == sunlight( today_sunset + 15_minutes ) );
        CHECK( 50.50f == sunlight( today_sunset + 30_minutes ) );
        CHECK( 25.75f == sunlight( today_sunset + 45_minutes ) );
        // 1 second before full night
        CHECK( 1.0275f == sunlight( today_sunset + 1_hours - 1_seconds ) );
        CHECK( 1.0f == sunlight( today_sunset + 1_hours ) );
        // After dusk
        CHECK( 1.0f == sunlight( today_sunset + 2_hours ) );
        CHECK( 1.0f == sunlight( today_sunset + 3_hours ) );
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
                CHECK( 1.0f == sunlight( new_moon ) );
            }
        }

        WHEN( "the moon is full" ) {
            REQUIRE( get_moon_phase( full_moon ) == MOON_FULL );
            THEN( "moonlight is 10.0" ) {
                CHECK( 10.0f == sunlight( full_moon ) );
            }
        }
    }
}

TEST_CASE( "sun math", "[sun]" )
{

    REQUIRE( get_option<int>( "LATITUDE" ) == 42 );

    // CDDA year is 364 days long but the math is for IRL 365 days. Causes some rounding errors as dates don't map exactly right.

    // January 1st 12:00
    static const time_point january_date = calendar::turn_zero + 12_hours;

    // Somewhere in September at 19:00
    static const time_point september_date = calendar::turn_zero + 260_days + 19_hours;

    // Somewhere in September at 03:17
    static const time_point september_date_2 = calendar::turn_zero + 260_days + 3_hours + 17_minutes;

    // Somewhere in June at 23:00
    static const time_point june_date = calendar::turn_zero + 160_days + 23_hours;

    // Somewhere in June at 23:00 on year 3
    static const time_point june_date_year3 = june_date + 2 * 364_days;

    SECTION( "solar hour angle" ) {
        CHECK( to_degrees( solar_hour_angle( january_date ) ) ==  Approx( 0 ).margin( 0.01 ) );
        CHECK( to_degrees( solar_hour_angle( september_date ) ) ==  Approx( 105 ).margin( 0.01 ) );
        CHECK( to_degrees( solar_hour_angle( september_date_2 ) ) ==  Approx( -130.75 ).margin( 0.01 ) );
        CHECK( to_degrees( solar_hour_angle( june_date ) ) ==  Approx( 165 ).margin( 0.01 ) );
        CHECK( to_degrees( solar_hour_angle( june_date_year3 ) ) ==  Approx( 165 ).margin( 0.01 ) );
    }

    SECTION( "sun declination" ) {
        CHECK( to_degrees( solar_declination( january_date ) ) ==  Approx( -23.02 ).margin( 0.01 ) );
        CHECK( to_degrees( solar_declination( september_date ) ) ==  Approx( 0.70 ).margin( 0.01 ) );
        CHECK( to_degrees( solar_declination( september_date_2 ) ) ==  Approx( 1.10 ).margin( 0.01 ) );
        CHECK( to_degrees( solar_declination( june_date ) ) ==  Approx( 23.05 ).margin( 0.01 ) );
        CHECK( to_degrees( solar_declination( june_date_year3 ) ) ==  Approx( 23.05 ).margin( 0.01 ) );
    }

    SECTION( "sun altitude" ) {
        CHECK( to_degrees( solar_altitude( january_date ) ) ==  Approx( 24.97 ).margin( 0.01 ) );
        CHECK( to_degrees( solar_altitude( september_date ) ) ==  Approx( -10.60 ).margin( 0.01 ) );
        CHECK( to_degrees( solar_altitude( september_date_2 ) ) ==  Approx( -28.16 ).margin( 0.01 ) );
        CHECK( to_degrees( solar_altitude( june_date ) ) ==  Approx( -23.47 ).margin( 0.01 ) );
        CHECK( to_degrees( solar_altitude( june_date_year3 ) ) ==  Approx( -23.47 ).margin( 0.01 ) );
    }

    SECTION( "angle at 8 morning" ) {
        const units::angle angle = units::from_degrees( 8 );
        CHECK( to_string( sun_at_angle( angle, january_date,
                                        false ) ) ==  "Year 1, Spring, day 1 8:05:48 AM" );
        CHECK( to_string( sun_at_angle( angle, september_date,
                                        false ) ) ==  "Year 1, Autumn, day 79 6:29:26 AM" );
        CHECK( to_string( sun_at_angle( angle, september_date_2,
                                        false ) ) ==  "Year 1, Autumn, day 79 6:27:58 AM" );
        CHECK( to_string( sun_at_angle( angle, june_date,
                                        false ) ) ==  "Year 1, Summer, day 70 5:03:29 AM" );
        CHECK( to_string( sun_at_angle( angle, june_date_year3,
                                        false ) ) ==  "Year 3, Summer, day 70 5:03:29 AM" );
    }

    SECTION( "angle at 8 evening" ) {
        const units::angle angle = units::from_degrees( 8 );
        CHECK( to_string( sun_at_angle( angle, january_date,
                                        true ) ) ==  "Year 1, Spring, day 1 3:54:11 PM" );
        CHECK( to_string( sun_at_angle( angle, september_date,
                                        true ) ) ==  "Year 1, Autumn, day 79 5:30:33 PM" );
        CHECK( to_string( sun_at_angle( angle, september_date_2,
                                        true ) ) ==  "Year 1, Autumn, day 79 5:32:01 PM" );
        CHECK( to_string( sun_at_angle( angle, june_date,
                                        true ) ) ==  "Year 1, Summer, day 70 6:56:30 PM" );
        CHECK( to_string( sun_at_angle( angle, june_date_year3,
                                        true ) ) ==  "Year 3, Summer, day 70 6:56:30 PM" );
    }
    }

    SECTION( "sunrise" ) {
        CHECK( to_string( sunrise( january_date ) ) ==  "Year 1, Spring, day 1 7:29:58 AM" );
        CHECK( to_string( sunrise( september_date ) ) ==  "Year 1, Autumn, day 79 5:57:27 AM" );
        CHECK( to_string( sunrise( september_date_2 ) ) ==  "Year 1, Autumn, day 79 5:56:00 AM" );
        CHECK( to_string( sunrise( june_date ) ) ==  "Year 1, Summer, day 70 4:29:51 AM" );
        CHECK( to_string( sunrise( june_date_year3 ) ) ==  "Year 3, Summer, day 70 4:29:51 AM" );
    }

    SECTION( "sunset" ) {
        CHECK( to_string( sunset( january_date ) ) ==  "Year 1, Spring, day 1 4:30:01 PM" );
        CHECK( to_string( sunset( september_date ) ) ==  "Year 1, Autumn, day 79 6:02:32 PM" );
        CHECK( to_string( sunset( september_date_2 ) ) ==  "Year 1, Autumn, day 79 6:03:59 PM" );
        CHECK( to_string( sunset( june_date ) ) ==  "Year 1, Summer, day 70 7:30:08 PM" );
        CHECK( to_string( sunset( june_date_year3 ) ) ==  "Year 3, Summer, day 70 7:30:08 PM" );
    }

    SECTION( "daylight" ) {
        // Starts at -12 degrees
        CHECK( to_string( daylight_time( january_date ) ) ==  "Year 1, Spring, day 1 6:40:14 AM" );
        CHECK( to_string( daylight_time( september_date ) ) ==  "Year 1, Autumn, day 79 5:09:23 AM" );
        CHECK( to_string( daylight_time( september_date_2 ) ) ==  "Year 1, Autumn, day 79 5:07:54 AM" );
        CHECK( to_string( daylight_time( june_date ) ) ==  "Year 1, Summer, day 70 3:35:02 AM" );
        CHECK( to_string( daylight_time( june_date_year3 ) ) ==  "Year 3, Summer, day 70 3:35:02 AM" );
    }

    SECTION( "night start" ) {
        // Starts at -12 degrees
        CHECK( to_string( night_time( january_date ) ) ==  "Year 1, Spring, day 1 5:19:45 PM" );
        CHECK( to_string( night_time( september_date ) ) ==  "Year 1, Autumn, day 79 6:50:36 PM" );
        CHECK( to_string( night_time( september_date_2 ) ) ==  "Year 1, Autumn, day 79 6:52:05 PM" );
        CHECK( to_string( night_time( june_date ) ) ==  "Year 1, Summer, day 70 8:24:57 PM" );
        CHECK( to_string( night_time( june_date_year3 ) ) ==  "Year 3, Summer, day 70 8:24:57 PM" );
    }
}

TEST_CASE( "sun angles at different latitudes", "[sun]" )
{
    // CDDA year is 364 days long but the math is for IRL 365 days. Causes some rounding errors as dates don't map exactly right.

    // Northward equinox March 20 12:00 approximately
    static const time_point northward_equinox = calendar::turn_zero + 110_days + 12_hours;

    // Summer solstice  June 21 12:00 approximately
    static const time_point summer_solstice = calendar::turn_zero + 171_days + 12_hours;

    // Shouthward equinox  september 22 12:00 approximately
    static const time_point southward_equinox = calendar::turn_zero + 262_days + 12_hours;

    // Winter solstice  December 21 12:00 approximately
    // "Year 1, Winter, day 81 12:00:00AM"
    static const time_point winter_solstice = calendar::turn_zero + 354_days + 12_hours;



    SECTION( "90 degrees pole" ) {
        override_option latitude( "LATITUDE", "90" );

        // Sun below horizont during midday on winter solistice
        // Sun above horizont durin midnight on summer solistice
        // Returns something valid for day/night/dawn/dusk times even if they don't happen


        CHECK( to_degrees( solar_altitude( winter_solstice ) ) ==  Approx( -23.43 ).margin( 0.01 ) );
        CHECK( to_degrees( solar_altitude( summer_solstice ) ) ==  Approx( 23.43 ).margin( 0.01 ) );

        // No sunsets during polar night or midnight sun
        // These functions instead return either previous or next midnight
        CHECK( to_string( sunset( winter_solstice ) ) ==  "Year 1, Winter, day 83 12:00:00AM" );
        CHECK( to_string( sunrise( winter_solstice ) ) ==  "Year 1, Winter, day 82 12:00:00AM" );

        CHECK( to_string( sunrise( summer_solstice ) ) ==  "Year 1, Summer, day 81 12:00:00AM" );
        CHECK( to_string( sunset( summer_solstice ) ) ==  "Year 1, Summer, day 82 12:00:00AM" );

        CHECK( to_string( night_time( winter_solstice ) ) ==  "Year 1, Winter, day 83 12:00:00AM" );
        CHECK( to_string( daylight_time( winter_solstice ) ) ==  "Year 1, Winter, day 82 12:00:00AM" );

        CHECK( to_string( daylight_time( summer_solstice ) ) ==  "Year 1, Summer, day 81 12:00:00AM" );
        CHECK( to_string( night_time( summer_solstice ) ) ==  "Year 1, Summer, day 82 12:00:00AM" );

        // Sunrise and dawn on equoinx at 6:00
        CHECK( to_string( sunset( northward_equinox ) ) ==  "Year 1, Summer, day 21 12:00:00AM" );
        CHECK( to_string( sunset( southward_equinox ) ) ==  "Year 1, Autumn, day 82 12:00:00AM" );

    }

    SECTION( "65 degrees below artic cirle" ) {
        override_option latitude( "LATITUDE", "90" );

        // Sunrise and sunset should happen normally


        CHECK( to_degrees( solar_altitude( winter_solstice ) ) ==  Approx( -23.43 ).margin( 0.01 ) );
        CHECK( to_degrees( solar_altitude( summer_solstice ) ) ==  Approx( 23.43 ).margin( 0.01 ) );

        // No sunsets during polar night or midnight sun
        // These functions instead return either previous or next midnight
        CHECK( to_string( sunset( winter_solstice ) ) ==  "Year 1, Winter, day 83 12:00:00AM" );
        CHECK( to_string( sunrise( winter_solstice ) ) ==  "Year 1, Winter, day 82 12:00:00AM" );

        CHECK( to_string( sunrise( summer_solstice ) ) ==  "Year 1, Summer, day 81 12:00:00AM" );
        CHECK( to_string( sunset( summer_solstice ) ) ==  "Year 1, Summer, day 82 12:00:00AM" );

        CHECK( to_string( night_time( winter_solstice ) ) ==  "Year 1, Winter, day 83 12:00:00AM" );
        CHECK( to_string( daylight_time( winter_solstice ) ) ==  "Year 1, Winter, day 82 12:00:00AM" );

        CHECK( to_string( daylight_time( summer_solstice ) ) ==  "Year 1, Summer, day 81 12:00:00AM" );
        CHECK( to_string( night_time( summer_solstice ) ) ==  "Year 1, Summer, day 82 12:00:00AM" );

        // Sunrise and dawn on equoinx at 6:00
        CHECK( to_string( sunset( northward_equinox ) ) ==  "Year 1, Summer, day 21 12:00:00AM" );
        CHECK( to_string( sunset( southward_equinox ) ) ==  "Year 1, Autumn, day 82 12:00:00AM" );

    }

    SECTION( "0 degrees equator" ) {
        override_option latitude( "LATITUDE", "0" );
        // Sunrise and dawn on equoinx at 6:00

        CHECK( to_string( sunrise( northward_equinox ) ) ==  "Year 1, Summer, day 20 6:00:00 AM" );
        CHECK( to_string( sunset( northward_equinox ) ) ==  "Year 1, Summer, day 20 6:00:00 PM" );

        CHECK( to_string( sunrise( southward_equinox ) ) ==  "Year 1, Autumn, day 81 6:00:00 AM" );
        CHECK( to_string( sunset( southward_equinox ) ) ==  "Year 1, Autumn, day 81 6:00:00 PM" );
    }

}
