#include "weather_gen.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ostream>
#include <random>
#include <string>

#include "cata_utility.h"
#include "game_constants.h"
#include "json.h"
#include "math_defines.h"
#include "point.h"
#include "rng.h"
#include "simplexnoise.h"
#include "weather.h"

namespace
{
constexpr double tau = 2 * M_PI;
constexpr double coldest_hour = 5;
// Out of 24 hours
constexpr double daily_magnitude_K = 5;
// Greatest absolute change from a day's average temperature, in kelvins
constexpr double seasonality_magnitude_K = 15;
// Greatest absolute change from the year's average temperature, in kelvins
constexpr double noise_magnitude_K = 8;
// Greatest absolute day-to-day noise, in kelvins
} //namespace

weather_generator::weather_generator() = default;
int weather_generator::current_winddir = 1000;

struct weather_gen_common {
    double x = 0;
    double y = 0;
    double z = 0;
    double cyf = 0;
    unsigned modSEED = 0u;
    season_type season = season_type::SPRING;
};

static weather_gen_common get_common_data( const tripoint &location, const time_point &t,
        unsigned seed )
{
    weather_gen_common result;
    // Integer x position / widening factor of the Perlin function.
    result.x = location.x / 2000.0;
    // Integer y position / widening factor of the Perlin function.
    result.y = location.y / 2000.0;
    // Integer turn / widening factor of the Perlin function.
    result.z = to_days<double>( t - calendar::turn_zero );
    // Limit the random seed during noise calculation, a large value flattens the noise generator to zero
    // Windows has a rand limit of 32768, other operating systems can have higher limits
    result.modSEED = seed % SIMPLEX_NOISE_RANDOM_SEED_LIMIT;
    const double year_fraction( time_past_new_year( t ) /
                                calendar::year_length() ); // [0,1)

    result.cyf = std::cos( tau * ( year_fraction + .125 ) ); // [-1, 1]
    // We add one-eighth to line up `cyf` so that 1 is at
    // midwinter and -1 at midsummer. (Cataclsym DDA years
    // start when spring starts. Gregorian years start when
    // winter starts.)
    result.season = season_of_year( t );

    return result;
}

static double weather_temperature_from_common_data( const weather_generator &wg,
        const weather_gen_common &common, const time_point &t )
{
    const double x( common.x );
    const double y( common.y );
    const double z( common.z );

    const unsigned modSEED = common.modSEED;
    const double seasonality = -common.cyf;
    // -1 in midwinter, +1 in midsummer
    const season_type season = common.season;
    const double dayFraction = time_past_midnight( t ) / 1_days;
    const double dayv = std::cos( tau * ( dayFraction + .5 - coldest_hour / 24 ) );
    // -1 at coldest_hour, +1 twelve hours later

    // manually specified seasonal temp variation from region_settings.json
    const int seasonal_temp_mod[4] = { wg.spring_temp_manual_mod, wg.summer_temp_manual_mod, wg.autumn_temp_manual_mod, wg.winter_temp_manual_mod };
    const double baseline(
        wg.base_temperature +
        seasonal_temp_mod[season] +
        dayv * daily_magnitude_K +
        seasonality * seasonality_magnitude_K );

    const double T = baseline + raw_noise_4d( x, y, z, modSEED ) * noise_magnitude_K;

    // Convert from Celsius to Fahrenheit
    return T * 9 / 5 + 32;
}

double weather_generator::get_weather_temperature( const tripoint &location, const time_point &t,
        unsigned seed ) const
{
    return weather_temperature_from_common_data( *this, get_common_data( location, t, seed ), t );
}
w_point weather_generator::get_weather( const tripoint &location, const time_point &t,
                                        unsigned seed ) const
{
    const weather_gen_common common = get_common_data( location, t, seed );

    const double x( common.x );
    const double y( common.y );
    const double z( common.z );

    const unsigned modSEED = common.modSEED;
    const double cyf( common.cyf );
    const double seasonality = -common.cyf;
    // -1 in midwinter, +1 in midsummer
    const season_type season = common.season;

    // Noise factors
    const double T( weather_temperature_from_common_data( *this, common, t ) );
    double W( raw_noise_4d( x / 2.5, y / 2.5, z / 200, modSEED ) * 10.0 );

    // Humidity variation
    double mod_h( 0 );
    if( season == WINTER ) {
        mod_h += winter_humidity_manual_mod;
    } else if( season == SPRING ) {
        mod_h += spring_humidity_manual_mod;
    } else if( season == SUMMER ) {
        mod_h += summer_humidity_manual_mod;
    } else if( season == AUTUMN ) {
        mod_h += autumn_humidity_manual_mod;
    }
    // Relative humidity, a percentage.
    double H = std::min( 100., std::max( 0.,
                                         base_humidity + mod_h + 100 * (
                                                 .15 * seasonality +
                                                 raw_noise_4d( x, y, z, modSEED + 101 ) *
                                                 .2 * ( -seasonality + 2 ) ) ) );

    // Pressure
    double P =
        base_pressure +
        raw_noise_4d( x, y, z, modSEED + 211 ) *
        10 * ( -seasonality + 2 );

    // Wind power
    W = std::max( 0, static_cast<int>( base_wind * rng( 1, 2 ) / std::pow( ( P + W ) / 1014.78, rng( 9,
                                       base_wind_distrib_peaks ) ) +
                                       -cyf / base_wind_season_variation * rng( 1, 2 ) ) );
    // Initial static variable
    if( current_winddir == 1000 ) {
        current_winddir = get_wind_direction( season );
        current_winddir = convert_winddir( current_winddir );
    } else {
        // When wind strength is low, wind direction is more variable
        bool changedir = one_in( W * 2160 );
        if( changedir ) {
            current_winddir = get_wind_direction( season );
            current_winddir = convert_winddir( current_winddir );
        }
    }
    std::string wind_desc = get_wind_desc( W );
    return w_point{ T, H, P, W, wind_desc, current_winddir, t };
}

weather_type_id weather_generator::get_weather_conditions( const tripoint &location,
        const time_point &t, unsigned seed,
        std::map<weather_type_id, time_point> &next_instance_allowed ) const
{
    w_point w( get_weather( location, t, seed ) );
    weather_type_id wt = get_weather_conditions( w, next_instance_allowed );
    return wt;
}

weather_type_id weather_generator::get_weather_conditions( const w_point &w,
        std::map<weather_type_id, time_point> &next_instance_allowed ) const
{
    weather_type_id current_conditions = WEATHER_CLEAR;
    for( const std::string &weather_type : weather_types ) {
        weather_type_id type = weather_type_id( weather_type );

        const weather_requirements &requires = type->requirements;
        if( ( w.time < ( calendar::start_of_cataclysm + requires.time_passed_min ) ) ||
            ( requires.time_passed_max != 0_seconds &&
              ( w.time > ( calendar::start_of_cataclysm + requires.time_passed_max ) ) ) ) {
            continue;
        }
        std::map<weather_type_id, time_point>::iterator instance = next_instance_allowed.find( type );
        if( instance != next_instance_allowed.end() && instance->second > calendar::turn ) {
            continue;
        }

        bool test_pressure =
            requires.pressure_max > w.pressure &&
            requires.pressure_min < w.pressure;
        bool test_humidity =
            requires.humidity_max > w.humidity &&
            requires.humidity_min < w.humidity;
        if( ( requires.humidity_and_pressure && !( test_pressure && test_humidity ) ) ||
            ( !requires.humidity_and_pressure && !( test_pressure || test_humidity ) ) ) {
            continue;
        }

        bool test_temperature =
            requires.temperature_max > w.temperature &&
            requires.temperature_min < w.temperature;
        bool test_windspeed =
            requires.windpower_max > w.windpower &&
            requires.windpower_min < w.windpower;
        if( !( test_temperature && test_windspeed ) ) {
            continue;
        }

        if( !requires.required_weathers.empty() ) {
            if( std::find( requires.required_weathers.begin(), requires.required_weathers.end(),
                           current_conditions ) == requires.required_weathers.end() ) {
                continue;
            }
        }

        if( !( requires.time == weather_time_requirement_type::both ||
               ( requires.time == weather_time_requirement_type::day && is_day( calendar::turn ) ) ||
               ( requires.time == weather_time_requirement_type::night && !is_day( calendar::turn ) ) ) ) {
            continue;
        }
        if( requires.one_in_chance != 0 && !one_in( requires.one_in_chance ) ) {
            continue;
        }

        current_conditions = type;
    }
    return current_conditions;
}

int weather_generator::get_wind_direction( const season_type season ) const
{
    cata_default_random_engine &wind_dir_gen = rng_get_engine();
    // Assign chance to angle direction
    if( season == SPRING ) {
        std::discrete_distribution<int> distribution {3, 3, 5, 8, 11, 10, 5, 2, 5, 6, 6, 5, 8, 10, 8, 6};
        return distribution( wind_dir_gen );
    } else if( season == SUMMER ) {
        std::discrete_distribution<int> distribution {3, 4, 4, 8, 8, 9, 8, 3, 7, 8, 10, 7, 7, 7, 5, 3};
        return distribution( wind_dir_gen );
    } else if( season == AUTUMN ) {
        std::discrete_distribution<int> distribution {4, 6, 6, 7, 6, 5, 4, 3, 5, 6, 8, 8, 10, 10, 8, 5};
        return distribution( wind_dir_gen );
    } else if( season == WINTER ) {
        std::discrete_distribution<int> distribution {5, 3, 2, 3, 2, 2, 2, 2, 4, 6, 10, 8, 12, 19, 13, 9};
        return distribution( wind_dir_gen );
    } else {
        return 0;
    }
}

int weather_generator::convert_winddir( const int inputdir ) const
{
    // Convert from discrete distribution output to angle
    float finputdir = inputdir * 22.5f;
    return static_cast<int>( finputdir );
}

int weather_generator::get_water_temperature() const
{
    /**
    WATER TEMPERATURE
    source : http://echo2.epfl.ch/VICAIRE/mod_2/chapt_5/main.htm
    source : http://www.grandriver.ca/index/document.cfm?Sec=2&Sub1=7&sub2=1
    **/

    int season_length = to_days<int>( calendar::season_length() );
    int day = to_days<int>( time_past_new_year( calendar::turn ) );
    int hour = hour_of_day<int>( calendar::turn );

    int water_temperature = 0;

    if( season_length == 0 ) {
        season_length = 1;
    }

    // Temperature varies between 33.8F and 75.2F depending on the time of year. Day = 0 corresponds to the start of spring.
    int annual_mean_water_temperature = 54.5 + 20.7 * std::sin( tau * ( day - season_length * 0.5 ) /
                                        ( season_length * 4.0 ) );
    // Temperature varies between +2F and -2F depending on the time of day. Hour = 0 corresponds to midnight.
    int daily_water_temperature_varaition = 2.0 + 2.0 * std::sin( tau * ( hour - 6.0 ) / 24.0 );

    water_temperature = annual_mean_water_temperature + daily_water_temperature_varaition;

    return water_temperature;
}

void weather_generator::test_weather( unsigned seed,
                                      std::map<weather_type_id, time_point> &next_instance_allowed ) const
{
    // Outputs a Cata year's worth of weather data to a CSV file.
    // Usage:
    // weather_generator WEATHERGEN; // Instantiate the class.
    // WEATHERGEN.test_weather(); // Runs this test.
    write_to_file( "weather.output", [&]( std::ostream & testfile ) {
        testfile <<
                 "|;year;season;day;hour;minute;temperature(F);humidity(%);pressure(mB);weatherdesc;windspeed(mph);winddirection"
                 << std::endl;

        const time_point begin = calendar::turn;
        const time_point end = begin + 2 * calendar::year_length();
        for( time_point i = begin; i < end; i += 20_minutes ) {
            w_point w = get_weather( tripoint_zero, to_turn<int>( i ), seed );
            weather_type_id conditions = get_weather_conditions( w, next_instance_allowed );

            int year = to_turns<int>( i - calendar::turn_zero ) / to_turns<int>
                       ( calendar::year_length() ) + 1;
            const int hour = hour_of_day<int>( i );
            const int minute = minute_of_hour<int>( i );
            int day;
            if( calendar::eternal_season() ) {
                day = to_days<int>( time_past_new_year( i ) );
            } else {
                day = day_of_season<int>( i );
            }
            testfile << "|;" << year << ";" << season_of_year( i ) << ";" << day << ";" << hour << ";" << minute
                     << ";" << w.temperature << ";" << w.humidity << ";" << w.pressure << ";" << conditions->name << ";"
                     <<
                     w.windpower << ";" << w.winddirection << std::endl;
        }

    }, "weather test file" );
}

weather_generator weather_generator::load( const JsonObject &jo )
{
    weather_generator ret;
    ret.base_temperature = jo.get_float( "base_temperature", 0.0 );
    ret.base_humidity = jo.get_float( "base_humidity", 50.0 );
    ret.base_pressure = jo.get_float( "base_pressure", 0.0 );
    ret.base_wind = jo.get_float( "base_wind", 0.0 );
    ret.base_wind_distrib_peaks = jo.get_int( "base_wind_distrib_peaks", 0 );
    ret.base_wind_season_variation = jo.get_int( "base_wind_season_variation", 0 );
    ret.summer_temp_manual_mod = jo.get_int( "summer_temp_manual_mod", 0 );
    ret.spring_temp_manual_mod = jo.get_int( "spring_temp_manual_mod", 0 );
    ret.autumn_temp_manual_mod = jo.get_int( "autumn_temp_manual_mod", 0 );
    ret.winter_temp_manual_mod = jo.get_int( "winter_temp_manual_mod", 0 );
    ret.spring_humidity_manual_mod = jo.get_int( "spring_humidity_manual_mod", 0 );
    ret.summer_humidity_manual_mod = jo.get_int( "summer_humidity_manual_mod", 0 );
    ret.autumn_humidity_manual_mod = jo.get_int( "autumn_humidity_manual_mod", 0 );
    ret.winter_humidity_manual_mod = jo.get_int( "winter_humidity_manual_mod", 0 );
    ret.weather_types = jo.get_string_array( "weather_types" );
    if( ret.weather_types.size() < 2 ) {
        jo.throw_error( "Need at least 2 weather types per region for null and default." );
    }
    return ret;
}
