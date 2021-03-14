#include <string>
#include <array>
#include <list>
#include <memory>

#include "avatar.h"
#include "catch/catch.hpp"
#include "player.h"
#include "weather.h"
#include "bodypart.h"
#include "calendar.h"
#include "item.h"
#include "player_helpers.h"
#include "map.h"
#include "map_helpers.h"
#include "weather.h"
#include "game.h"
#include "units.h"
#include "hash_utils.h"
#include "overmapbuffer.h"

struct temperature_threshold {
    constexpr temperature_threshold( int v, const char *n )
        : value( v )
        , name( n )
    {}

    int value;
    const char *name;
};

#define t(x) temperature_threshold(x, #x)

constexpr std::array<temperature_threshold, 7> bodytemps = {{
        t( BODYTEMP_FREEZING ), t( BODYTEMP_VERY_COLD ), t( BODYTEMP_COLD ),
        t( BODYTEMP_NORM ),
        t( BODYTEMP_HOT ), t( BODYTEMP_VERY_HOT ), t( BODYTEMP_SCORCHING )
    }
};

#undef t

// Run update_bodytemp() until core body temperature settles.
static int converge_temperature( player &p, size_t iters, int start_temperature = BODYTEMP_NORM )
{
    REQUIRE( get_weather().weather == WEATHER_CLOUDY );
    REQUIRE( get_weather().windspeed == 0 );

    for( int i = 0 ; i < num_bp; i++ ) {
        p.temp_cur[i] = start_temperature;
    }
    for( int i = 0 ; i < num_bp; i++ ) {
        p.temp_conv[i] = start_temperature;
    }

    bool converged = false;

    using temp_array = decltype( p.temp_cur );
    std::unordered_set<temp_array, cata::range_hash> history( iters );

    for( size_t i = 0; i < iters; i++ ) {
        if( history.count( p.temp_cur ) != 0 ) {
            converged = true;
            break;
        }

        history.emplace( p.temp_cur );
        p.update_bodytemp();
    }

    CAPTURE( iters );
    CAPTURE( p.temp_cur );
    // If it doesn't converge, it's usually very close to it anyway, so don't fail
    CHECK( converged );
    return p.temp_cur[0];
}

static void equip_clothing( player &p, const std::vector<std::string> &clothing )
{
    for( const std::string &c : clothing ) {
        const item article( c, 0 );
        p.wear_item( article );
    }
}

// TODO: Find good name
/**
 * Table of temperature ranges closest to given body temperature.
 * That is, [0] to [1] is the range where FREEZING is the closest.
 */
static std::array<int, 8> bodytemp_voronoi()
{
    std::array<int, 8> midpoints;
    midpoints[0] = INT_MIN;
    midpoints[7] = INT_MAX;
    for( int i = 0; i < 6; i++ ) {
        midpoints[i + 1] = ( bodytemps[i].value + bodytemps[i + 1].value ) / 2;
    }

    return midpoints;
}

// Run the tests for each of the temperature setpoints.
static void test_temperature_spread( player &p,
                                     const std::array<units::temperature, 7> &air_temperatures )
{
    const auto thresholds = bodytemp_voronoi();
    for( int i = 0; i < 7; i++ ) {
        get_weather().temperature = to_fahrenheit( air_temperatures[i] );
        get_weather().clear_temp_cache();
        CAPTURE( air_temperatures[i] );
        CAPTURE( get_weather().temperature );
        int converged_temperature = converge_temperature( p, 1000, bodytemps[i].value );
        auto expected_body_temperature = bodytemps[i].name;
        CAPTURE( expected_body_temperature );
        int air_temperature_celsius = to_celsius( air_temperatures[i] );
        CAPTURE( air_temperature_celsius );
        int min_temperature = thresholds[i];
        int max_temperature = thresholds[i + 1];
        CHECK( converged_temperature > min_temperature );
        CHECK( converged_temperature < max_temperature );
    }
}

const std::vector<std::string> light_clothing = {{
        "hat_ball",
        "bandana",
        "tshirt",
        "gloves_fingerless",
        "jeans",
        "socks",
        "sneakers"
    }
};

const std::vector<std::string> heavy_clothing = {{
        "hat_knit",
        "tshirt",
        "vest",
        "trenchcoat",
        "gloves_wool",
        "long_underpants",
        "pants_army",
        "socks_wool",
        "boots"
    }
};

const std::vector<std::string> arctic_clothing = {{
        "balclava",
        "goggles_ski",
        "hat_hunting",
        "under_armor",
        "vest",
        "coat_winter",
        "gloves_liner",
        "gloves_winter",
        "long_underpants",
        "pants_fur",
        "socks_wool",
        "boots_winter",
    }
};

static void guarantee_neutral_weather( const player &p )
{
    get_weather().weather = WEATHER_CLOUDY;
    get_weather().weather_override = WEATHER_CLOUDY;
    get_weather().windspeed = 0;
    get_weather().weather_precise->humidity = 0;
    REQUIRE( !get_map().has_flag( TFLAG_SWIMMABLE, p.pos() ) );
    REQUIRE( !get_map().has_flag( TFLAG_DEEP_WATER, p.pos() ) );
    REQUIRE( !g->is_in_sunlight( p.pos() ) );

    const w_point weather = *g->weather.weather_precise;
    const oter_id &cur_om_ter = overmap_buffer.ter( p.global_omt_location() );
    bool sheltered = g->is_sheltered( p.pos() );
    double total_windpower = get_local_windpower( g->weather.windspeed, cur_om_ter,
                             p.pos(),
                             g->weather.winddirection, sheltered );
    int air_humidity = get_local_humidity( weather.humidity, g->weather.weather,
                                           sheltered );
    REQUIRE( air_humidity == 0 );
    REQUIRE( total_windpower == 0.0 );
    REQUIRE( !const_cast<player &>( p ).in_climate_control() );
    REQUIRE( !p.can_use_floor_warmth() );
    REQUIRE( get_heat_radiation( p.pos(), true ) == 0 );
    REQUIRE( p.body_wetness == decltype( p.body_wetness )() );
}

TEST_CASE( "Player body temperatures within expected bounds.", "[bodytemp][slow]" )
{
    clear_map();
    clear_avatar();
    player &dummy = get_avatar();
    guarantee_neutral_weather( dummy );

    SECTION( "Nude target temperatures." ) {
        test_temperature_spread( dummy, {{-26_C, -11_C, 11_C, 26_C, 41_C, 56_C, 71_C,}} );
    }

    SECTION( "Lightly clothed target temperatures" ) {
        equip_clothing( dummy, light_clothing );
        test_temperature_spread( dummy, {{-29_C, -14_C, 1_C, 24_C, 39_C, 54_C, 69_C,}} );
    }

    SECTION( "Heavily clothed target temperatures" ) {
        equip_clothing( dummy, heavy_clothing );
        test_temperature_spread( dummy, {{-46_C, -30_C, -11_C, 8_C, 33_C, 48_C, 63_C,}} );
    }

    SECTION( "Arctic gear target temperatures" ) {
        equip_clothing( dummy, arctic_clothing );
        test_temperature_spread( dummy, {{-83_C, -68_C, -50_C, -24_C, 3_C, 27_C, 43_C,}} );
    }
}

/**
 * Finds air temperatures for which body temperature is closest to exact "named" value.
 * FREEZING, HOT, etc.
 */
static std::array<units::temperature, bodytemps.size()> find_temperature_points( player &p )
{
    std::array<std::pair<int, int>, bodytemps.size()> value_distances;
    std::fill( value_distances.begin(), value_distances.end(), std::make_pair( 0, INT_MAX ) );
    int last_converged_temperature = INT_MIN;
    for( int i = -200; i < 200; i++ ) {
        get_weather().temperature = i;
        get_weather().clear_temp_cache();
        CAPTURE( units::from_fahrenheit( i ) );
        int converged_temperature = converge_temperature( p, 10000 );
        CHECK( converged_temperature >= last_converged_temperature );
        // 0 - FREEZING, 6 - SCORCHING
        for( size_t temperature_index = 0; temperature_index < bodytemps.size(); temperature_index++ ) {
            int distance_to_definition = std::abs( bodytemps[temperature_index].value - converged_temperature );
            if( distance_to_definition < value_distances[temperature_index].second ) {
                value_distances[temperature_index] = std::make_pair( i, distance_to_definition );
            }
        }
        last_converged_temperature = converged_temperature;
    }

    std::array<units::temperature, 7> points;
    std::transform( value_distances.begin(), value_distances.end(), points.begin(),
    []( const std::pair<int, int> &pr ) {
        // Round it to nearest full Celsius, for nicer display
        return units::from_celsius( std::round( units::fahrenheit_to_celsius( pr.first ) ) );
    } );

    auto sorted_copy = points;
    std::sort( sorted_copy.begin(), sorted_copy.end() );
    CHECK( sorted_copy == points );
    return points;
}

static void print_temperatures( const std::array<units::temperature, bodytemps.size()>
                                &temperatures )
{
    std::string s = "{{";
    for( auto &t : temperatures ) {
        s += to_string( units::to_celsius( t ) ) + "_C,";
    }
    s += "}}\n";
    cata_printf( s );
}

TEST_CASE( "Find air temperatures for given body temperatures.", "[.][bodytemp]" )
{
    clear_map();
    clear_avatar();
    player &dummy = get_avatar();
    guarantee_neutral_weather( dummy );

    SECTION( "Nude target temperatures." ) {
        const auto points = find_temperature_points( dummy );
        print_temperatures( points );
    }

    SECTION( "Lightly clothed target temperatures" ) {
        equip_clothing( dummy, light_clothing );
        const auto points = find_temperature_points( dummy );
        print_temperatures( points );
    }

    SECTION( "Heavily clothed target temperatures" ) {
        equip_clothing( dummy, heavy_clothing );
        const auto points = find_temperature_points( dummy );
        print_temperatures( points );
    }

    SECTION( "Arctic gear target temperatures" ) {
        equip_clothing( dummy, arctic_clothing );
        const auto points = find_temperature_points( dummy );
        print_temperatures( points );
    }
}

// Ugly pasta, for simplicity
static int find_converging_water_temp( player &p, int expected_water, int expected_bodytemp )
{
    constexpr int tol = 100;
    REQUIRE( get_map().has_flag( TFLAG_SWIMMABLE, p.pos() ) );
    REQUIRE( get_map().has_flag( TFLAG_DEEP_WATER, p.pos() ) );
    int actual_water = expected_water;
    int step = 2 * 128;
    do {
        step /= 2;
        get_weather().water_temperature = actual_water;
        get_weather().clear_temp_cache();
        const int actual_temperature = get_weather().get_water_temperature( p.pos() );
        REQUIRE( actual_temperature == actual_water );

        int converged_temperature = converge_temperature( p, 10000 );
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

TEST_CASE( "Player body temperatures in water.", "[.][bodytemp]" )
{
    clear_map();
    clear_avatar();
    player &dummy = get_avatar();

    const tripoint &pos = dummy.pos();

    get_map().ter_set( pos, t_water_dp );
    REQUIRE( get_map().has_flag( TFLAG_SWIMMABLE, pos ) );
    REQUIRE( get_map().has_flag( TFLAG_DEEP_WATER, pos ) );
    REQUIRE( !g->is_in_sunlight( pos ) );
    get_weather().weather = WEATHER_CLOUDY;

    dummy.drench( 100, body_part_set::all(), true );

    SECTION( "Nude target temperatures." ) {
        test_water_temperature_spread( dummy, {{ 38, 53, 70, 86, 102, 118, 135 }} );
    }

    // Not supposed to be very protective under water
    SECTION( "Arctic gear target temperatures" ) {
        equip_clothing( dummy, arctic_clothing );
        test_water_temperature_spread( dummy, {{ 25, 42, 57, 76, 96, 112, 128 }} );
    }

    // Should keep warmth under water
    SECTION( "Swimming gear target temperatures" ) {
        equip_clothing( dummy, { "wetsuit_hood", "wetsuit" } );
        test_water_temperature_spread( dummy, {{ 37, 54, 69, 86, 102, 118, 134 }} );
    }
}

static void hypothermia_check( player &p, int water_temperature, time_duration expected_time,
                               int expected_temperature )
{
    get_weather().water_temperature = water_temperature;
    get_weather().clear_temp_cache();
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

TEST_CASE( "Water hypothermia check.", "[.][bodytemp]" )
{
    clear_map();
    clear_avatar();
    player &dummy = get_avatar();

    const tripoint &pos = dummy.pos();

    get_map().ter_set( pos, t_water_dp );
    REQUIRE( get_map().has_flag( TFLAG_SWIMMABLE, pos ) );
    REQUIRE( get_map().has_flag( TFLAG_DEEP_WATER, pos ) );
    REQUIRE( !g->is_in_sunlight( pos ) );
    get_weather().weather = WEATHER_CLOUDY;

    dummy.drench( 100, body_part_set::all(), true );

    SECTION( "Cold" ) {
        hypothermia_check( dummy, units::celsius_to_fahrenheit( 20 ), 5_minutes, BODYTEMP_COLD );
    }

    SECTION( "Very cold" ) {
        hypothermia_check( dummy, units::celsius_to_fahrenheit( 10 ), 5_minutes, BODYTEMP_VERY_COLD );
    }

    SECTION( "Freezing" ) {
        hypothermia_check( dummy, units::celsius_to_fahrenheit( 0 ), 5_minutes, BODYTEMP_FREEZING );
    }
}
