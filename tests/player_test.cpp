#include <string>
#include <array>
#include <list>
#include <memory>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "player.h"
#include "weather.h"
#include "bodypart.h"
#include "calendar.h"
#include "item.h"
#include "player_helpers.h"
#include "map.h"
#include "map_helpers.h"
#include "weather.h"

// Run update_bodytemp() until core body temperature settles.
static int converge_temperature( player &p )
{
    for( int i = 0 ; i < num_bp; i++ ) {
        p.temp_cur[i] = BODYTEMP_NORM;
    }
    for( int i = 0 ; i < num_bp; i++ ) {
        p.temp_conv[i] = BODYTEMP_NORM;
    }

    int prev_temp = p.temp_cur[0];
    bool converged = false;
    // Run it once, to set the prev_ properly
    p.update_bodytemp();
    int prev_diff = prev_temp - p.temp_cur[0];

    // Doesn't always converge on one number, we need to check a sign flip
    for( int i = 0; i < 10000; i++ ) {
        int cur_diff = prev_temp - p.temp_cur[0];
        if( cur_diff == 0 || sgn( prev_diff ) != sgn( cur_diff ) ) {
            converged = true;
            break;
        } else if( prev_diff != cur_diff ) {
            prev_diff = cur_diff;
        }
        prev_temp = p.temp_cur[0];
        p.update_bodytemp();
    }

    CAPTURE( prev_temp );
    CAPTURE( p.temp_cur[0] );
    REQUIRE( converged );
    return p.temp_cur[0];
}

static int find_converging_temp( player &p, int expected_ambient, int expected_bodytemp )
{
    constexpr int tol = 100;
    REQUIRE( !g->m.has_flag( TFLAG_SWIMMABLE, p.pos() ) );
    REQUIRE( !g->m.has_flag( TFLAG_DEEP_WATER, p.pos() ) );
    int actual_ambient = expected_ambient;
    int step = 2 * 128;
    do {
        step /= 2;
        g->weather.temperature = actual_ambient;
        g->weather.clear_temp_cache();
        const int actual_temperature = g->weather.get_temperature( p.pos() );
        REQUIRE( actual_temperature == actual_ambient );

        int converged_temperature = converge_temperature( p );
        bool high_enough = expected_bodytemp - tol <= converged_temperature;
        bool low_enough  = expected_bodytemp + tol >= converged_temperature;
        if( high_enough && low_enough ) {
            return actual_ambient;
        }

        int direction = high_enough ? -1 : 1;
        actual_ambient += direction * step;
    } while( step > 0 );
    bool converged = step > 0;
    CHECK( converged );

    return actual_ambient;
}

static void equip_clothing( player &p, const std::string &clothing )
{
    const item article( clothing, 0 );
    p.wear_item( article );
}

// Run the tests for each of the temperature setpoints.
static void test_temperature_spread( player &p, const std::array<int, 7> &expected_temps )
{
    std::array<int, 7> actual_temps;
    actual_temps[0] = find_converging_temp( p, expected_temps[0], BODYTEMP_FREEZING );
    actual_temps[1] = find_converging_temp( p, expected_temps[1], BODYTEMP_VERY_COLD );
    actual_temps[2] = find_converging_temp( p, expected_temps[2], BODYTEMP_COLD );
    actual_temps[3] = find_converging_temp( p, expected_temps[3], BODYTEMP_NORM );
    actual_temps[4] = find_converging_temp( p, expected_temps[4], BODYTEMP_HOT );
    actual_temps[5] = find_converging_temp( p, expected_temps[5], BODYTEMP_VERY_HOT );
    actual_temps[6] = find_converging_temp( p, expected_temps[6], BODYTEMP_SCORCHING );
    CHECK( actual_temps == expected_temps );
}

TEST_CASE( "Player body temperatures converge on expected values.", "[.bodytemp]" )
{
    clear_map();
    clear_avatar();
    player &dummy = g->u;

    const tripoint &pos = dummy.pos();

    REQUIRE( !g->m.has_flag( TFLAG_SWIMMABLE, pos ) );
    REQUIRE( !g->m.has_flag( TFLAG_DEEP_WATER, pos ) );
    REQUIRE( !g->is_in_sunlight( pos ) );
    g->weather.weather = WEATHER_CLOUDY;
    g->weather.windspeed = 0;

    SECTION( "Nude target temperatures." ) {
        test_temperature_spread( dummy, {{ 8, 35, 64, 82, 98, 110, 122 }} );
    }

    SECTION( "Lightly clothed target temperatures" ) {
        equip_clothing( dummy, "hat_ball" );
        equip_clothing( dummy, "bandana" );
        equip_clothing( dummy, "tshirt" );
        equip_clothing( dummy, "gloves_fingerless" );
        equip_clothing( dummy, "jeans" );
        equip_clothing( dummy, "socks" );
        equip_clothing( dummy, "sneakers" );

        test_temperature_spread( dummy, {{ 3, 30, 59, 80, 97, 110, 121 }} );
    }

    SECTION( "Heavily clothed target temperatures" ) {
        equip_clothing( dummy, "hat_knit" );
        equip_clothing( dummy, "tshirt" );
        equip_clothing( dummy, "vest" );
        equip_clothing( dummy, "trenchcoat" );
        equip_clothing( dummy, "gloves_wool" );
        equip_clothing( dummy, "long_underpants" );
        equip_clothing( dummy, "pants_army" );
        equip_clothing( dummy, "socks_wool" );
        equip_clothing( dummy, "boots" );

        test_temperature_spread( dummy, {{ -29, -2, 33, 68, 90, 104, 116 }} );
    }

    SECTION( "Arctic gear target temperatures" ) {
        equip_clothing( dummy, "balclava" );
        equip_clothing( dummy, "goggles_ski" );
        equip_clothing( dummy, "hat_hunting" );
        equip_clothing( dummy, "under_armor" );
        equip_clothing( dummy, "vest" );
        equip_clothing( dummy, "coat_winter" );
        equip_clothing( dummy, "gloves_liner" );
        equip_clothing( dummy, "gloves_winter" );
        equip_clothing( dummy, "long_underpants" );
        equip_clothing( dummy, "pants_fur" );
        equip_clothing( dummy, "socks_wool" );
        equip_clothing( dummy, "boots_winter" );

        test_temperature_spread( dummy, {{ -95, -70, -35, 12, 62, 84, 98 }} );
    }
}

// Ugly pasta, for simplicity
static int find_converging_water_temp( player &p, int expected_water, int expected_bodytemp )
{
    constexpr int tol = 100;
    REQUIRE( g->m.has_flag( TFLAG_SWIMMABLE, p.pos() ) );
    REQUIRE( g->m.has_flag( TFLAG_DEEP_WATER, p.pos() ) );
    int actual_water = expected_water;
    int step = 2 * 128;
    do {
        step /= 2;
        g->weather.water_temperature = actual_water;
        g->weather.clear_temp_cache();
        const int actual_temperature = g->weather.get_water_temperature( p.pos() );
        REQUIRE( actual_temperature == actual_water );

        int converged_temperature = converge_temperature( p );
        bool high_enough = expected_bodytemp - tol <= converged_temperature;
        bool low_enough  = expected_bodytemp + tol >= converged_temperature;
        if( high_enough && low_enough ) {
            return actual_water;
        }

        int direction = high_enough ? -1 : 1;
        actual_water += direction * step;
    } while( step > 0 );
    bool converged = step > 0;
    CHECK( converged );

    return actual_water;
}

// Also ugly pasta, also for simplicity
static void test_water_temperature_spread( player &p, const std::array<int, 7> &expected_temps )
{
    std::array<int, 7> actual_temps;
    actual_temps[0] = find_converging_water_temp( p, expected_temps[0], BODYTEMP_FREEZING );
    actual_temps[1] = find_converging_water_temp( p, expected_temps[1], BODYTEMP_VERY_COLD );
    actual_temps[2] = find_converging_water_temp( p, expected_temps[2], BODYTEMP_COLD );
    actual_temps[3] = find_converging_water_temp( p, expected_temps[3], BODYTEMP_NORM );
    actual_temps[4] = find_converging_water_temp( p, expected_temps[4], BODYTEMP_HOT );
    actual_temps[5] = find_converging_water_temp( p, expected_temps[5], BODYTEMP_VERY_HOT );
    actual_temps[6] = find_converging_water_temp( p, expected_temps[6], BODYTEMP_SCORCHING );
    CHECK( actual_temps == expected_temps );
}

TEST_CASE( "Player body temperatures in water.", "[.bodytemp]" )
{
    clear_map();
    clear_avatar();
    player &dummy = g->u;

    const tripoint &pos = dummy.pos();

    g->m.ter_set( pos, t_water_dp );
    REQUIRE( g->m.has_flag( TFLAG_SWIMMABLE, pos ) );
    REQUIRE( g->m.has_flag( TFLAG_DEEP_WATER, pos ) );
    REQUIRE( !g->is_in_sunlight( pos ) );
    g->weather.weather = WEATHER_CLOUDY;

    dummy.drench( 100, body_part_set::all(), true );

    SECTION( "Nude target temperatures." ) {
        test_water_temperature_spread( dummy, {{ 38, 53, 70, 86, 102, 118, 135 }} );
    }

    // Not supposed to be very protective under water
    SECTION( "Arctic gear target temperatures" ) {
        equip_clothing( dummy, "balclava" );
        equip_clothing( dummy, "goggles_ski" );
        equip_clothing( dummy, "hat_hunting" );
        equip_clothing( dummy, "under_armor" );
        equip_clothing( dummy, "vest" );
        equip_clothing( dummy, "coat_winter" );
        equip_clothing( dummy, "gloves_liner" );
        equip_clothing( dummy, "gloves_winter" );
        equip_clothing( dummy, "long_underpants" );
        equip_clothing( dummy, "pants_fur" );
        equip_clothing( dummy, "socks_wool" );
        equip_clothing( dummy, "boots_winter" );

        test_water_temperature_spread( dummy, {{ 25, 42, 57, 76, 96, 112, 128 }} );
    }

    // Should keep warmth under water
    SECTION( "Swimming gear target temperatures" ) {
        equip_clothing( dummy, "wetsuit_hood" );
        equip_clothing( dummy, "wetsuit" );

        test_water_temperature_spread( dummy, {{ 37, 54, 69, 86, 102, 118, 134 }} );
    }
}

static void hypothermia_check( player &p, int water_temperature, time_duration expected_time,
                               int expected_temperature )
{
    g->weather.water_temperature = water_temperature;
    g->weather.clear_temp_cache();
    int expected_turns = to_turns<int>( expected_time );
    int lower_bound = expected_turns * 0.8f;
    int upper_bound = expected_turns * 1.2f;

    int actual_time;
    for( actual_time = 0; actual_time < upper_bound * 2; actual_time++ ) {
        p.update_bodytemp();
        if( p.temp_cur[0] <= expected_temperature ) {
            break;
        }
    }

    CHECK( actual_time >= lower_bound );
    CHECK( actual_time <= upper_bound );
    CHECK( p.temp_cur[0] <= expected_temperature );
}

TEST_CASE( "Water hypothermia check.", "[.bodytemp]" )
{
    clear_map();
    clear_avatar();
    player &dummy = g->u;

    const tripoint &pos = dummy.pos();

    g->m.ter_set( pos, t_water_dp );
    REQUIRE( g->m.has_flag( TFLAG_SWIMMABLE, pos ) );
    REQUIRE( g->m.has_flag( TFLAG_DEEP_WATER, pos ) );
    REQUIRE( !g->is_in_sunlight( pos ) );
    g->weather.weather = WEATHER_CLOUDY;

    dummy.drench( 100, body_part_set::all(), true );

    SECTION( "Cold" ) {
        hypothermia_check( dummy, celsius_to_fahrenheit( 20 ), 5_minutes, BODYTEMP_COLD );
    }

    SECTION( "Very cold" ) {
        hypothermia_check( dummy, celsius_to_fahrenheit( 10 ), 5_minutes, BODYTEMP_VERY_COLD );
    }

    SECTION( "Freezing" ) {
        hypothermia_check( dummy, celsius_to_fahrenheit( 0 ), 5_minutes, BODYTEMP_FREEZING );
    }
}
