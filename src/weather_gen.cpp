#include "weather_gen.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <random>
#include <string>

#include "enums.h"
#include "game_constants.h"
#include "json.h"
#include "rng.h"
#include "simplexnoise.h"
#include "weather.h"

namespace
{
// GCC doesn't like M_PI here for some reason
constexpr double PI  = 3.141592653589793238463;
constexpr double tau = 2 * PI;
} //namespace

weather_generator::weather_generator() = default;
int weather_generator::current_winddir = 1000;

w_point weather_generator::get_weather( const tripoint &location, const time_point &t,
                                        unsigned seed ) const
{
    // Integer x position / widening factor of the Perlin function.
    const double x( location.x / 2000.0 );
    // Integer y position / widening factor of the Perlin function.
    const double y( location.y / 2000.0 );
    // Integer turn / widening factor of the Perlin function.
    const double z( to_turn<int>( t + calendar::season_length() ) / 2000.0 );

    const double dayFraction = time_past_midnight( t ) / 1_days;

    // Limit the random seed during noise calculation, a large value flattens the noise generator to zero
    // Windows has a rand limit of 32768, other operating systems can have higher limits
    const unsigned modSEED = seed % SIMPLEX_NOISE_RANDOM_SEED_LIMIT;
    // Noise factors
    double T( raw_noise_4d( x, y, z, modSEED ) * 4.0 );
    double H( raw_noise_4d( x, y, z / 5, modSEED + 101 ) );
    double H2( raw_noise_4d( x, y, z, modSEED + 151 ) / 4 );
    double P( raw_noise_4d( x / 2.5, y / 2.5, z / 30, modSEED + 211 ) * 70 );
    double A( raw_noise_4d( x, y, z, modSEED ) * 8.0 );
    double W( raw_noise_4d( x / 2.5, y / 2.5, z / 200, modSEED ) * 10.0 );

    const double now( ( time_past_new_year( t ) + calendar::season_length() / 2 ) /
                      calendar::year_length() ); // [0,1)
    const double ctn( cos( tau * now ) );
    const season_type season = season_of_year( now );
    // Temperature variation
    // TODO: make this depend on latitude and altitude?
    const double mod_t( 0 );
    // Current baseline temperature. Degrees Celsius.
    const double current_t( base_temperature + mod_t );
    // Start and end at -1 going up to 1 in summer.
    const double seasonal_variation( ctn * -1 );
    // Harsh winter nights, hot summers.
    const double season_atenuation( ctn / 2 + 1 );
    // Make summers peak faster and winters not perma-frozen.
    const double season_dispersion( pow( 2, ctn + 1 ) - 2.3 );
    // Day-night temperature variation.
    const double daily_variation( cos( tau * dayFraction - tau / 8 ) * -1 * season_atenuation / 2 +
                                  season_dispersion * -1 );

    // Add baseline to the noise.
    T += current_t;
    // Add season curve offset to account for the winter-summer difference in day-night difference.
    T += seasonal_variation * 8 * exp( -pow( current_t * 2.7 / 10 - 0.5, 2 ) );
    // Add daily variation scaled to the inverse of the current baseline. A very specific and finicky adjustment curve.
    T += daily_variation * 8 * exp( -pow( current_t / 30, 2 ) );
    // Convert to imperial. =|
    T = T * 9 / 5 + 32;

    // Humidity variation
    const double mod_h( 0 );
    const double current_h( base_humidity + mod_h );
    // Humidity stays mostly at the mean level, but has low peaks rarely. It's a percentage.
    H = std::max( std::min( ( ctn / 10.0 + ( -pow( H, 2 ) * 3 + H2 ) ) * current_h / 2.0 + current_h,
                            100.0 ), 0.0 );

    // Pressure variation
    // Pressure is mostly random, but a bit higher on summer and lower on winter. In millibars.
    P += seasonal_variation * 20 + base_pressure;

    // Wind power
    W = std::max( 0, static_cast<int>( base_wind  / pow( P / 1014.78, rng( 9,
                                       base_wind_distrib_peaks ) ) +
                                       seasonal_variation / base_wind_season_variation * rng( 1, 2 ) * W ) );
    // Wind direction
    // Initial static variable
    if( current_winddir == 1000 ) {
        current_winddir = get_wind_direction( season, seed );
        current_winddir = convert_winddir( current_winddir );
    } else {
        // When wind strength is low, wind direction is more variable
        bool changedir = one_in( W * 360 );
        if( changedir ) {
            current_winddir = get_wind_direction( season, seed );
            current_winddir = convert_winddir( current_winddir );
        }
    }
    std::string wind_desc = get_wind_desc( W );
    // Acid rains
    const double acid_content = base_acid * A;
    bool acid = acid_content >= 1.0;
    return w_point {T, H, P, W, wind_desc, current_winddir, acid};
}

weather_type weather_generator::get_weather_conditions( const tripoint &location,
        const time_point &t, unsigned seed ) const
{
    w_point w( get_weather( location, t, seed ) );
    weather_type wt = get_weather_conditions( w );
    // Make sure we don't say it's sunny at night! =P
    if( wt == WEATHER_SUNNY && calendar( to_turn<int>( t ) ).is_night() ) {
        return WEATHER_CLEAR;
    }
    return wt;
}

weather_type weather_generator::get_weather_conditions( const w_point &w ) const
{
    weather_type r( WEATHER_CLEAR );
    if( w.pressure > 1030 && w.humidity < 70 ) {
        r = WEATHER_SUNNY;
    }
    if( w.pressure < 1030 && w.humidity > 40 ) {
        r = WEATHER_CLOUDY;
    }
    if( r == WEATHER_CLOUDY && ( w.humidity > 60 || w.pressure < 1010 ) ) {
        r = WEATHER_DRIZZLE;
    }
    if( r == WEATHER_DRIZZLE && ( w.humidity > 70 || w.pressure < 1000 ) ) {
        r = WEATHER_RAINY;
    }
    if( r == WEATHER_RAINY && w.pressure < 985 ) {
        r = WEATHER_THUNDER;
    }
    if( r == WEATHER_THUNDER && w.pressure < 970 ) {
        r = WEATHER_LIGHTNING;
    }

    if( w.temperature <= 32 ) {
        if( r == WEATHER_DRIZZLE ) {
            r = WEATHER_FLURRIES;
        } else if( r > WEATHER_DRIZZLE ) {
            r = WEATHER_SNOW;
        }
        if( r == WEATHER_SNOW && w.pressure < 960 && w.windpower > 15 ) {
            r = WEATHER_SNOWSTORM;
        }
    }

    if( r == WEATHER_DRIZZLE && w.acidic ) {
        r = WEATHER_ACID_DRIZZLE;
    }
    if( r > WEATHER_DRIZZLE && w.acidic ) {
        r = WEATHER_ACID_RAIN;
    }
    return r;
}

int weather_generator::get_wind_direction( const season_type season, unsigned seed ) const
{
    unsigned dirseed = seed;
    std::default_random_engine wind_dir_gen( dirseed );
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
    float finputdir = inputdir * 22.5;
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
    int day = calendar::turn.day_of_year();
    int hour = hour_of_day<int>( calendar::turn );

    int water_temperature = 0;

    if( season_length == 0 ) {
        season_length = 1;
    }

    // Temperature varies between 33.8F and 75.2F depending on the time of year. Day = 0 corresponds to the start of spring.
    int annual_mean_water_temperature = 54.5 + 20.7 * sin( tau * ( day - season_length * 0.5 ) /
                                        ( season_length * 4.0 ) );
    // Temperature varies between +2F and -2F depending on the time of day. Hour = 0 corresponds to midnight.
    int daily_water_temperature_varaition = 2.0 + 2.0 * sin( tau * ( hour - 6.0 ) / 24.0 );

    water_temperature = annual_mean_water_temperature + daily_water_temperature_varaition;

    return water_temperature;
}

void weather_generator::test_weather() const
{
    // Outputs a Cata year's worth of weather data to a CSV file.
    // Usage:
    // weather_generator WEATHERGEN; // Instantiate the class.
    // WEATHERGEN.test_weather(); // Runs this test.
    std::ofstream testfile;
    testfile.open( "weather.output", std::ofstream::trunc );
    testfile <<
             "|;year;season;day;hour;minute;temperature(F);humidity(%);pressure(mB);weatherdesc;windspeed(mph);winddirection"
             << std::endl;

    const time_point begin = calendar::turn;
    const time_point end = begin + 2 * calendar::year_length();
    for( time_point i = begin; i < end; i += 200_turns ) {
        w_point w = get_weather( tripoint_zero, to_turn<int>( i ), 1000 );
        weather_type c =  get_weather_conditions( w );
        weather_datum wd = weather_data( c );

        int year = to_turns<int>( i - calendar::time_of_cataclysm ) / to_turns<int>
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
                 << ";" << w.temperature << ";" << w.humidity << ";" << w.pressure << ";" << wd.name << ";" <<
                 w.windpower << ";" << w.winddirection << std::endl;
    }
}

weather_generator weather_generator::load( JsonObject &jo )
{
    weather_generator ret;
    ret.base_temperature = jo.get_float( "base_temperature", 6.5 );
    ret.base_humidity = jo.get_float( "base_humidity", 66.0 );
    ret.base_pressure = jo.get_float( "base_pressure", 1015.0 );
    ret.base_acid = jo.get_float( "base_acid", 1015.0 );
    ret.base_wind = jo.get_float( "base_wind", 5.7 );
    ret.base_wind_distrib_peaks = jo.get_int( "base_wind_distrib_peaks", 30 );
    ret.base_wind_season_variation = jo.get_int( "base_wind_season_variation", 64 );
    return ret;
}
