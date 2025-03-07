#include "calendar.h" // IWYU pragma: associated

#include <array>
#include <cstdio>
#include <functional>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_set>

#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "hash_utils.h"
#include "line.h"
#include "options_helpers.h"
#include "units.h"
#include "units_utility.h"

// SUN TESTS

// The 24-hour solar cycle has four overlapping parts, as defined by four calendar.cpp functions:
//
// is_night : While the Sun is below -6° altitude
// is_day   : While the Sun is above -1° altitude
// is_dawn, is_dusk : While the Sun is in between -6° to -1° at the appropriate end
//                    of the day
//
//
// The times of sunrise and sunset will naturally depend on the current time of year; this aspect is
// covered by the "sunrise and sunset" and solstice/equinox tests later in this file. Here we simply
// use the first day of spring as a baseline.
TEST_CASE( "daily_solar_cycle", "[sun][night][dawn][day][dusk]" )
{
    // Use sunrise/sunset on the first day (spring equinox)
    const time_point midnight = calendar::turn_zero;
    const time_point noon = calendar::turn_zero + 12_hours;
    const time_point today_sunrise = sunrise( midnight );
    const time_point today_sunset = sunset( midnight );

    CAPTURE( to_string( today_sunrise ) );
    CAPTURE( to_string( today_sunset ) );

    SECTION( "Night" ) {
        // First, at the risk of stating the obvious
        CHECK( is_night( midnight + 1_seconds ) );
        CHECK( is_night( midnight + 2_hours ) );
        CHECK( is_night( midnight + 3_hours ) );
        CHECK( is_night( midnight + 4_hours ) );

        // Yep, still dark
        CHECK( is_night( midnight + 1_seconds ) );
        CHECK( is_night( midnight + 2_hours ) );
        CHECK( is_night( midnight + 3_hours ) );
        CHECK( is_night( midnight + 4_hours ) );
    }

    SECTION( "Dawn" ) {
        CHECK_FALSE( is_night( today_sunrise ) );
        CHECK( is_dawn( today_sunrise - 1_seconds ) );
        CHECK( is_dawn( today_sunrise - 20_minutes ) );

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
        CHECK_FALSE( is_day( today_sunset + 1_seconds ) );
        CHECK( is_dusk( today_sunset + 1_seconds ) );
        CHECK( is_dusk( today_sunset + 20_minutes ) );
    }

    SECTION( "Night again" ) {
        CHECK( is_night( today_sunset + 2_hours ) );
        CHECK( is_night( today_sunset + 3_hours ) );
        CHECK( is_night( today_sunset + 4_hours ) );
    }

    SECTION( "Eternal night" ) {
        //Eternal night
        calendar::set_eternal_night( true );
        CHECK( is_night( midnight + 1_seconds ) );
        CHECK( is_night( today_sunrise + 1_seconds ) );
        CHECK( is_night( noon + 1_seconds ) );
        CHECK( is_night( today_sunset + 1_seconds ) );
        CHECK_FALSE( is_dawn( today_sunrise + 1_seconds ) );
        CHECK_FALSE( is_day( noon + 1_seconds ) );
        CHECK_FALSE( is_dusk( today_sunset + 1_seconds ) );
        calendar::set_eternal_night( false );
    }

    SECTION( "Eternal night" ) {
        //Eternal day
        calendar::set_eternal_day( true );
        CHECK( is_day( midnight + 1_seconds ) );
        CHECK( is_day( today_sunrise + 1_seconds ) );
        CHECK( is_day( noon + 1_seconds ) );
        CHECK( is_day( today_sunset + 1_seconds ) );
        CHECK_FALSE( is_dawn( today_sunrise + 1_seconds ) );
        CHECK_FALSE( is_night( midnight + 1_seconds ) );
        CHECK_FALSE( is_dusk( today_sunset + 1_seconds ) );
        calendar::set_eternal_day( false );
    }
}

// The calendar `sunlight` function returns light level for both sun and moon.
TEST_CASE( "sunlight_and_moonlight", "[sun][sunlight][moonlight]" )
{
    // Use sunrise/sunset on the first day (spring equinox)
    const time_point midnight = calendar::turn_zero;
    const time_point noon = calendar::turn_zero + 12_hours;
    const time_point today_sunrise = sunrise( midnight );
    const time_point today_sunset = sunset( midnight );
    CHECK( today_sunset > today_sunrise );
    CHECK( today_sunrise > midnight );

    // Expected numbers below assume 110.0f maximum daylight level
    // (maximum daylight is different at other times of year - see [daylight] tests)
    REQUIRE( 100.0f == default_daylight_level() );

    SECTION( "sunlight" ) {
        // Before dawn
        CHECK( 1.0f == sun_moon_light_at( midnight ) );
        // Dawn
        CHECK( sun_moon_light_at( today_sunrise - 2_hours ) == 1.0f );
        CHECK( sun_moon_light_at( today_sunrise - 1_hours ) == Approx( 5 ).margin( 2 ) );
        CHECK( sun_moon_light_at( today_sunrise ) == Approx( 60 ).margin( 1 ) );
        // Light gets brighter towards noon
        CHECK( sun_moon_light_at( today_sunrise + 2_hours ) >
               sun_moon_light_at( today_sunrise + 1_hours ) );
        CHECK( sun_moon_light_at( today_sunrise + 3_hours ) >
               sun_moon_light_at( today_sunrise + 2_hours ) );
        // Noon
        CHECK( sun_moon_light_at( noon ) == Approx( 110 ).margin( 10 ) );
        CHECK( sun_moon_light_at( noon + 1_hours ) <
               sun_moon_light_at( noon ) );
        CHECK( sun_moon_light_at( noon + 2_hours ) <
               sun_moon_light_at( noon + 1_hours ) );
        // Dusk begins
        CHECK( sun_moon_light_at( today_sunset - 1_hours ) ==
               Approx( sun_moon_light_at( today_sunrise + 1_hours ) ).margin( 1 ) );
        CHECK( sun_moon_light_at( today_sunset ) ==
               Approx( sun_moon_light_at( today_sunrise ) ).margin( 1 ) );
        CHECK( sun_moon_light_at( today_sunset + 1_hours ) ==
               Approx( sun_moon_light_at( today_sunrise - 1_hours ) ).margin( 1 ) );

        // Eternal night
        calendar::set_eternal_night( true );
        CHECK( sun_moon_light_at( midnight ) == 1.0f );
        CHECK( sun_moon_light_at( today_sunset ) == 1.0f );
        CHECK( sun_moon_light_at( today_sunrise ) == 1.0f );
        CHECK( sun_moon_light_at( noon ) == 1.0f );
        calendar::set_eternal_night( false );
        // Eternal day
        calendar::set_eternal_day( true );
        CHECK( sun_moon_light_at( midnight ) == Approx( 126 ).margin( 5 ) );
        CHECK( sun_moon_light_at( today_sunset ) == Approx( 126 ).margin( 5 ) );
        CHECK( sun_moon_light_at( today_sunrise ) == Approx( 126 ).margin( 5 ) );
        CHECK( sun_moon_light_at( noon ) == Approx( 126 ).margin( 5 ) );
        calendar::set_eternal_day( false );
    }

    // This moonlight test is intentionally simple, only checking new moon (minimal light) and full
    // moon (maximum moonlight). More detailed tests of moon phase and light should be expressed in
    // `moon_test.cpp`. Including here simply to check that `sun_moon_light_at` also calculates
    // moonlight.
    SECTION( "moonlight" ) {
        const time_duration phase_time = calendar::season_length() / 6;
        const time_point new_moon = calendar::turn_zero;
        const time_point full_moon = new_moon + phase_time;
        const time_point full_moon_midnight = full_moon - time_past_midnight( full_moon );

        WHEN( "the moon is new" ) {
            REQUIRE( get_moon_phase( new_moon ) == MOON_NEW );
            THEN( "moonlight is 1.0" ) {
                CHECK( 1.0f == sun_moon_light_at( new_moon ) );
            }
        }

        WHEN( "the moon is full" ) {
            REQUIRE( get_moon_phase( full_moon_midnight ) == MOON_FULL );
            THEN( "moonlight is 7.0" ) {
                CHECK( 7.0f == sun_moon_light_at( full_moon_midnight ) );
            }
        }
    }
}

// sanity-check seasonally-adjusted maximum daylight level
TEST_CASE( "noon_sunlight_levels", "[sun][daylight][equinox][solstice]" )
{
    const time_duration one_season = calendar::season_length();
    const time_point spring = calendar::turn_zero;
    const time_point summer = spring + one_season;
    const time_point autumn = summer + one_season;
    const time_point winter = autumn + one_season;

    SECTION( "baseline 110 daylight on the spring and autumn equinoxes" ) {
        float spring_light = sun_light_at( spring + 12_hours );
        CHECK( spring_light == Approx( 110.0f ).margin( 10 ) );
        CHECK( sun_light_at( autumn + 12_hours ) == Approx( spring_light ).margin( 1 ) );
    }

    SECTION( "125 daylight on the summer solstice" ) {
        CHECK( sun_light_at( summer + 12_hours ) == 125.0f );
    }

    SECTION( "90 daylight on the winter solstice" ) {
        CHECK( sun_light_at( winter + 12_hours ) == Approx( 87.0f ).margin( 10 ) );
    }

    // Many other times of day have peak daylight level, but noon is for sure
    SECTION( "noon is peak daylight level" ) {
        CHECK( sun_moon_light_at( spring + 12_hours ) ==
               Approx( sun_moon_light_at_noon_near( spring ) ).margin( 3 ) );
        CHECK( sun_moon_light_at( summer + 12_hours ) ==
               Approx( sun_moon_light_at_noon_near( summer ) ).margin( 3 ) );
        CHECK( sun_moon_light_at( autumn + 12_hours ) ==
               Approx( sun_moon_light_at_noon_near( autumn ) ).margin( 3 ) );
        CHECK( sun_moon_light_at( winter + 12_hours ) ==
               Approx( sun_moon_light_at_noon_near( winter ) ).margin( 3 ) );
    }

    SECTION( "Eternal day" ) {
        // locked sunlight when eternal day id due
        calendar::set_eternal_day( true );
        CHECK( sun_light_at( spring + 12_hours ) == 125.0f );
        CHECK( sun_light_at( summer + 12_hours ) == 125.0f );
        CHECK( sun_light_at( autumn + 12_hours ) == 125.0f );
        CHECK( sun_light_at( winter + 12_hours ) == 125.0f );
        calendar::set_eternal_day( false );
    }
}

// The times of sunrise and sunset vary throughout the year. Equinoxes occur on the
// first day of spring and autumn, and solstices occur on the first day of summer and winter.
TEST_CASE( "sunrise_and_sunset", "[sun][sunrise][sunset][equinox][solstice]" )
{
    // Due to the "NN_days" math below, this test requires a default 91-day season length
    REQUIRE( calendar::season_from_default_ratio() == Approx( 1.0f ) );

    const time_duration one_season = calendar::season_length();
    const time_point spring = calendar::turn_zero;
    const time_point summer = spring + one_season;
    const time_point autumn = summer + one_season;
    const time_point winter = autumn + one_season;

    auto const sunrise_in_day = []( const time_point & t ) {
        return time_past_midnight( sunrise( t ) );
    };

    auto const sunset_in_day = []( const time_point & t ) {
        return time_past_midnight( sunset( t ) );
    };

    // The expected sunrise/sunset times depend on calculations in `calendar.cpp`
    // They should approximately match Boston in the year 2000.  There is minor
    // variation due to our year length being 364 days and some other minor
    // simplifications in the formulae.

    SECTION( "spring equinox is day 1 of spring" ) {
        // Actual sunrise and sunset on March 21st 2001 are 0545 and 1757
        CHECK( "Year 1, Spring, day 1 5:57:58 AM" == to_string( sunrise( spring ) ) );
        CHECK( "Year 1, Spring, day 1 6:16:07 PM" == to_string( sunset( spring ) ) );
    }

    SECTION( "summer solstice is day 1 of summer" ) {
        // Actual sunrise and sunset on June 21st 2001 are 0407 and 1924
        CHECK( "Year 1, Summer, day 1 4:22:20 AM" == to_string( sunrise( summer ) ) );
        CHECK( "Year 1, Summer, day 1 7:41:38 PM" == to_string( sunset( summer ) ) );
    }

    SECTION( "autumn equinox is day 1 of autumn" ) {
        // Actual sunrise and sunset on September 22nd 2001 are 0531 and 1741
        CHECK( "Year 1, Autumn, day 1 5:45:33 AM" == to_string( sunrise( autumn ) ) );
        CHECK( "Year 1, Autumn, day 1 5:59:37 PM" == to_string( sunset( autumn ) ) );
    }

    SECTION( "winter solstice is day 1 of winter" ) {
        // Actual sunrise and sunset on December 21st 2001 are 0710 and 1614
        CHECK( "Year 1, Winter, day 1 7:25:07 AM" == to_string( sunrise( winter ) ) );
        CHECK( "Year 1, Winter, day 1 4:31:46 PM" == to_string( sunset( winter ) ) );
    }

    SECTION( "spring sunrise gets earlier" ) {
        CHECK( sunrise_in_day( spring + 30_days ) < sunrise_in_day( spring ) );
        CHECK( sunrise_in_day( spring + 60_days ) < sunrise_in_day( spring + 30_days ) );
        CHECK( sunrise_in_day( spring + 90_days ) < sunrise_in_day( spring + 60_days ) );
    }
    SECTION( "spring sunset gets later" ) {
        CHECK( sunset_in_day( spring + 30_days ) > sunset_in_day( spring ) );
        CHECK( sunset_in_day( spring + 60_days ) > sunset_in_day( spring + 30_days ) );
        CHECK( sunset_in_day( spring + 90_days ) > sunset_in_day( spring + 60_days ) );
    }

    SECTION( "summer sunrise gets later" ) {
        CHECK( sunrise_in_day( summer + 30_days ) > sunrise_in_day( summer ) );
        CHECK( sunrise_in_day( summer + 60_days ) > sunrise_in_day( summer + 30_days ) );
        CHECK( sunrise_in_day( summer + 90_days ) > sunrise_in_day( summer + 60_days ) );
    }
    SECTION( "summer sunset gets earlier" ) {
        CHECK( sunset_in_day( summer + 30_days ) < sunset_in_day( summer ) );
        CHECK( sunset_in_day( summer + 60_days ) < sunset_in_day( summer + 30_days ) );
        CHECK( sunset_in_day( summer + 90_days ) < sunset_in_day( summer + 60_days ) );
    }

    SECTION( "autumn sunrise gets later" ) {
        CHECK( sunrise_in_day( autumn + 30_days ) > sunrise_in_day( autumn ) );
        CHECK( sunrise_in_day( autumn + 60_days ) > sunrise_in_day( autumn + 30_days ) );
        CHECK( sunrise_in_day( autumn + 90_days ) > sunrise_in_day( autumn + 60_days ) );
    }
    SECTION( "autumn sunset gets earlier" ) {
        CHECK( sunset_in_day( autumn + 30_days ) < sunset_in_day( autumn ) );
        CHECK( sunset_in_day( autumn + 60_days ) < sunset_in_day( autumn + 30_days ) );
        CHECK( sunset_in_day( autumn + 90_days ) < sunset_in_day( autumn + 60_days ) );
    }

    SECTION( "winter sunrise gets earlier" ) {
        CHECK( sunrise_in_day( winter + 30_days ) < sunrise_in_day( winter ) );
        CHECK( sunrise_in_day( winter + 60_days ) < sunrise_in_day( winter + 30_days ) );
        CHECK( sunrise_in_day( winter + 90_days ) < sunrise_in_day( winter + 60_days ) );
    }
    SECTION( "winter sunset gets later" ) {
        CHECK( sunset_in_day( winter + 30_days ) > sunset_in_day( winter ) );
        CHECK( sunset_in_day( winter + 60_days ) > sunset_in_day( winter + 30_days ) );
        CHECK( sunset_in_day( winter + 90_days ) > sunset_in_day( winter + 60_days ) );
    }
}

static rl_vec2d checked_sunlight_angle( const time_point &t )
{
    const std::optional<rl_vec2d> opt_angle = sunlight_angle( t );
    REQUIRE( opt_angle );
    return *opt_angle;
}

static constexpr time_point first_midnight = calendar::turn_zero;
static constexpr time_point first_noon = first_midnight + 12_hours;

TEST_CASE( "sun_highest_at_noon", "[sun]" )
{
    for( int i = 0; i < 100; ++i ) {
        CAPTURE( i );

        const time_point midnight = first_midnight + i * 1_days;
        CHECK_FALSE( sunlight_angle( midnight ) );

        const time_point noon = first_noon + i * 1_days;
        const time_point before_noon = noon - 2_hours;
        const time_point after_noon = noon + 2_hours;

        const rl_vec2d before_noon_angle = checked_sunlight_angle( before_noon );
        const rl_vec2d noon_angle = checked_sunlight_angle( noon );
        const rl_vec2d after_noon_angle = checked_sunlight_angle( after_noon );

        CAPTURE( before_noon_angle );
        CAPTURE( noon_angle );
        CAPTURE( after_noon_angle );
        // Sun should be highest around noon
        CHECK( noon_angle.magnitude() < before_noon_angle.magnitude() );
        CHECK( noon_angle.magnitude() < after_noon_angle.magnitude() );

        // Sun should always be in the South, meaning angle points North
        // (negative)
        CHECK( before_noon_angle.y < 0 );
        CHECK( noon_angle.y < 0 );
        CHECK( after_noon_angle.y < 0 );

        // Sun should be moving westwards across the sky, so its angle points
        // more eastwards, which means it's increasing
        CHECK( noon_angle.x > before_noon_angle.x );
        CHECK( after_noon_angle.x > noon_angle.x );

        CHECK( before_noon_angle.magnitude() ==
               Approx( after_noon_angle.magnitude() ).epsilon( 0.25 ) );
    }
}

TEST_CASE( "noon_sun_does_not_move_much", "[sun]" )
{
    rl_vec2d noon_angle = checked_sunlight_angle( first_noon );
    for( int i = 1; i < 1000; ++i ) {
        CAPTURE( i );
        const time_point later_noon = first_noon + i * 1_days;
        const rl_vec2d later_noon_angle = checked_sunlight_angle( later_noon );
        CHECK( noon_angle.x == Approx( later_noon_angle.x ).margin( 0.01 ) );
        CHECK( noon_angle.y == Approx( later_noon_angle.y ).epsilon( 0.05 ) );
        noon_angle = later_noon_angle;
    }
}

TEST_CASE( "dawn_dusk_fixed_during_eternal_season", "[sun]" )
{
    on_out_of_scope restore_eternal_season( []() {
        calendar::set_eternal_season( false );
    } );
    calendar::set_eternal_season( true );
    override_option override_eternal_season( "ETERNAL_SEASON", "true" );

    const time_point first_sunrise = sunrise( first_noon );
    const time_point first_sunset = sunset( first_noon );

    for( int i = 1; i < 1000; ++i ) {
        CAPTURE( i );
        const time_point this_noon = first_noon + i * 1_days;
        const time_point this_sunrise = sunrise( this_noon );
        const time_point this_sunset = sunset( this_noon );

        CHECK( this_sunrise < this_noon );
        CHECK( this_sunset > this_noon );

        CHECK( time_past_midnight( this_sunrise ) == time_past_midnight( first_sunrise ) );
        CHECK( time_past_midnight( this_sunset ) == time_past_midnight( first_sunset ) );
    }
}

TEST_CASE( "sun_altitude_fixed_during_eternal_night_or_day", "[sun]" )
{
    const time_point midnight = calendar::turn_zero;
    const time_point noon = calendar::turn_zero + 12_hours;
    const time_point today_sunrise = sunrise( midnight );
    const time_point today_sunset = sunset( midnight );

    // Eternal night
    calendar::set_eternal_night( true );
    CHECK( sun_azimuth_altitude( midnight ).second < -10_degrees );
    CHECK( sun_azimuth_altitude( today_sunset ).second < -10_degrees );
    CHECK( sun_azimuth_altitude( today_sunrise ).second < -10_degrees );
    CHECK( sun_azimuth_altitude( noon ).second < -10_degrees );
    calendar::set_eternal_night( false );

    // Eternal day
    calendar::set_eternal_day( true );
    CHECK( sun_azimuth_altitude( midnight ).second == 90_degrees );
    CHECK( sun_azimuth_altitude( today_sunset ).second == 90_degrees );
    CHECK( sun_azimuth_altitude( today_sunrise ).second == 90_degrees );
    CHECK( sun_azimuth_altitude( noon ).second == 90_degrees );
    calendar::set_eternal_day( false );
}

TEST_CASE( "sunrise_sunset_consistency", "[sun]" )
{
    bool set_eternal = GENERATE( false, true );
    on_out_of_scope restore_eternal_season( []() {
        calendar::set_eternal_season( false );
    } );
    calendar::set_eternal_season( set_eternal );

    for( int i = 1; i < 1000; ++i ) {
        CAPTURE( i );
        const time_point this_noon = first_noon + i * 1_days;
        {
            const time_point this_sunrise = sunrise( this_noon );
            CHECK( this_sunrise < this_noon );
            units::angle azimuth;
            units::angle altitude;
            std::tie( azimuth, altitude ) =
                sun_azimuth_altitude( this_sunrise );
            CHECK( to_degrees( altitude ) == Approx( -1 ).margin( 0.01 ) );
        }
        {
            const time_point this_sunset = sunset( this_noon );
            CHECK( this_sunset > this_noon );
            units::angle azimuth;
            units::angle altitude;
            std::tie( azimuth, altitude ) =
                sun_azimuth_altitude( this_sunset );
            CHECK( to_degrees( altitude ) == Approx( -1 ).margin( 0.01 ) );
        }
        {
            const time_point this_daylight = daylight_time( this_noon );
            CHECK( this_daylight < this_noon );
            units::angle azimuth;
            units::angle altitude;
            std::tie( azimuth, altitude ) =
                sun_azimuth_altitude( this_daylight );
            CHECK( to_degrees( altitude ) == Approx( -6 ).margin( 0.01 ) );
        }
    }
}

using PointSet = std::unordered_set<std::pair<int, int>, cata::tuple_hash>;

static PointSet sun_positions_regular( time_point start, time_point end, time_duration interval,
                                       int azimuth_scale )
{
    CAPTURE( to_days<int>( start - calendar::turn_zero ) );
    std::unordered_set<std::pair<int, int>, cata::tuple_hash> plot_points;

    for( time_point t = start; t < end; t += interval ) {
        CAPTURE( to_minutes<int>( t - start ) );
        units::angle azimuth;
        units::angle altitude;
        std::tie( azimuth, altitude ) = sun_azimuth_altitude( t );
        if( altitude < 0_degrees ) {
            continue;
        }
        // Convert to ASCII-art plot
        // x-axis is azimuth, 4/azimuth_scale degrees per column
        // y-axis is altitude, 3 degrees per column
        azimuth = normalize( azimuth + 180_degrees );
        // Scale azimuth away from 180 by specified scale
        azimuth = 180_degrees + ( azimuth - 180_degrees ) * azimuth_scale;
        REQUIRE( azimuth >= 0_degrees );
        REQUIRE( azimuth <= 360_degrees );
        REQUIRE( altitude >= 0_degrees );
        REQUIRE( altitude <= 90_degrees );
        plot_points.emplace( static_cast<int>( azimuth / 4_degrees ),
                             static_cast<int>( altitude / 3_degrees ) );
    }

    return plot_points;
}

static PointSet sun_throughout_day( time_point day_start )
{
    REQUIRE( time_past_midnight( day_start ) == 0_seconds );
    // Calculate the Sun's position every few minutes thourhgout the day
    time_point day_end = day_start + 1_days;
    return sun_positions_regular( day_start, day_end, 5_minutes, 1 );
}

static PointSet sun_throughout_year( time_point day_start )
{
    REQUIRE( time_past_midnight( day_start ) == 0_seconds );
    // Calculate the Sun's position every noon throughout the year
    time_point first_noon = day_start + 1_days / 2;
    time_point last_noon = first_noon + calendar::year_length();
    return sun_positions_regular( first_noon, last_noon, 1_days, 4 );
}

static void check_sun_plot( const std::vector<PointSet> &points, const std::string &reference )
{
    static constexpr std::array<char, 3> symbols = { { '#', '*', '-' } };
    REQUIRE( points.size() <= symbols.size() );

    std::ostringstream os;
    os << "Altitude\n";

    for( int rough_altitude = 30; rough_altitude >= 0; --rough_altitude ) {
        for( int rough_azimuth = 0; rough_azimuth <= 90; ++rough_azimuth ) {
            std::pair<int, int> p{ rough_azimuth, rough_altitude };
            char c = ' ';
            for( size_t i = 0; i < points.size(); ++i ) {
                if( points[i].count( p ) ) {
                    c = symbols[i];
                    break;
                }
            }
            os << c;
        }
        os << '\n';
    }
    os << std::setw( 92 ) << "Azimuth\n";
    std::string result = os.str();
    CHECK( result == reference );
    // When the test fails, print out something to copy-paste as a new
    // reference output:
    if( result != reference ) {
        result.pop_back();
        for( const std::string &line : string_split( result, '\n' ) ) {
            printf( R"("%s\n")" "\n", line.c_str() );
        }
    }
}

TEST_CASE( "movement_of_sun_through_day", "[sun]" )
{
    PointSet equinox_points = sun_throughout_day( calendar::turn_zero );
    PointSet summer_points =
        sun_throughout_day( calendar::turn_zero + calendar::season_length() );
    PointSet winter_points =
        sun_throughout_day( calendar::turn_zero + calendar::season_length() * 3 );
    std::string reference =
// *INDENT-OFF*
"Altitude\n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                     ################                                      \n"
"                                  ####              ####                                   \n"
"                               ####                     ##                                 \n"
"                              ##                          ##                               \n"
"                            ##                             ###                             \n"
"                           ##                                ##                            \n"
"                          ##                                  ##                           \n"
"                         ##               *******              ##                          \n"
"                        ##            ****      *****           ##                         \n"
"                        #          ***              ***          ##                        \n"
"                       #         ***                  ***         #                        \n"
"                      ##        **                      **        ##                       \n"
"                     ##        **                        **        ##                      \n"
"                     #        **                           *        #                      \n"
"                    ##       *                              *       ##                     \n"
"                   ##       *              ----              *       #                     \n"
"                   #       **          -----  -----           *       #                    \n"
"                  ##      **         ---          ---          *      ##                   \n"
"                 ##      **        ---              --         **      ##                  \n"
"                 #       *        --                  --        **      #                  \n"
"                ##      *        --                    --        *      ##                 \n"
"                #      **       --                      --       **      #                 \n"
"               #      **       --                        --       **      #                \n"
"              ##      *       --                          --       *       #               \n"
"                                                                                    Azimuth\n";
// *INDENT-ON*
    check_sun_plot( { summer_points, equinox_points, winter_points }, reference );
}

TEST_CASE( "movement_of_noon_through_year", "[sun]" )
{
    PointSet points = sun_throughout_year( calendar::turn_zero );
    std::string reference =
// *INDENT-OFF*
// This should yield an analemma
"Altitude\n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                         ######                                            \n"
"                                        ##    ##                                           \n"
"                                         ##   ##                                           \n"
"                                          ## ##                                            \n"
"                                           ###                                             \n"
"                                           ###                                             \n"
"                                           # ##                                            \n"
"                                          #   ##                                           \n"
"                                         ##    ##                                          \n"
"                                         #      ##                                         \n"
"                                        ##       #                                         \n"
"                                        #        #                                         \n"
"                                        #        #                                         \n"
"                                        ##       #                                         \n"
"                                         ##     ##                                         \n"
"                                          #######                                          \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                    Azimuth\n";
// *INDENT-ON*
    check_sun_plot( { points }, reference );
}

TEST_CASE( "noon_rises_towards_summer_solsitice_and_falls_towards_winter", "[sun]" )
{
    int old_season_length = to_days<int>( calendar::season_length() );
    on_out_of_scope restore_season_length( [ = ]() {
        calendar::set_season_length( old_season_length );
    } );
    int this_season_length = GENERATE( 5, 14, 91, 1000 );
    CAPTURE( this_season_length );
    calendar::set_season_length( this_season_length );

    const time_point summer_solstice = calendar::turn_zero + calendar::season_length();
    const time_point winter_solstice = calendar::turn_zero + 3 * calendar::season_length();

    // Make some allowance and don't check the days within this range of the
    // solstice.
    const time_duration allowance = calendar::season_length() / 100;

    rl_vec2d last_noon_angle;

    for( time_point noon = first_noon; noon < winter_solstice; noon += 1_days ) {
        CAPTURE( to_days<int>( noon - first_noon ) );

        const rl_vec2d noon_angle = checked_sunlight_angle( noon );

        if( last_noon_angle.magnitude() != 0 ) {
            CAPTURE( last_noon_angle );
            CAPTURE( noon_angle );

            if( noon < summer_solstice - allowance ) {
                // Sun should be higher than yesterday until summer solstice
                CHECK( noon_angle.magnitude() < last_noon_angle.magnitude() );
            } else if( noon >= summer_solstice + allowance &&
                       noon <= winter_solstice - allowance ) {
                // ...and then lower than yesterday until winter solstice
                CHECK( noon_angle.magnitude() > last_noon_angle.magnitude() );
            }
        }

        last_noon_angle = noon_angle;
    }
}
