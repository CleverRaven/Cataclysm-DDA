#include "weather_gen.h"
#include "weather.h"
#include "enums.h"
#include "calendar.h"
#include "simplexnoise.h"
#include "json.h"

#include <cmath>
#include <fstream>
#include <cstdlib>

namespace
{
// GCC doesn't like M_PI here for some reason
constexpr double PI  = 3.141592653589793238463;
constexpr double tau = 2 * PI;
} //namespace

weather_generator::weather_generator() = default;

w_point weather_generator::get_weather( const tripoint &location, const time_point &t,
                                        unsigned seed ) const
{
    const double x( location.x /
                    2000.0 ); // Integer x position / widening factor of the Perlin function.
    const double y( location.y /
                    2000.0 ); // Integer y position / widening factor of the Perlin function.
    const double z( to_turn<int>( t + calendar::season_length() ) /
                    2000.0 ); // Integer turn / widening factor of the Perlin function.

    const double dayFraction = time_past_midnight( t ) / 1_days;

    //limit the random seed during noise calculation, a large value flattens the noise generator to zero
    //Windows has a rand limit of 32768, other operating systems can have higher limits
    const unsigned modSEED = seed % 32768;

    // Noise factors
    double T( raw_noise_4d( x, y, z, modSEED ) * 4.0 );
    double H( raw_noise_4d( x, y, z / 5, modSEED + 101 ) );
    double H2( raw_noise_4d( x, y, z, modSEED + 151 ) / 4 );
    double P( raw_noise_4d( x, y, z / 3, modSEED + 211 ) * 70 );
    double A( raw_noise_4d( x, y, z, modSEED ) * 8.0 );
    double W;

    const double now( ( time_past_new_year( t ) + calendar::season_length() / 2 ) /
                      calendar::year_length() ); // [0,1)
    const double ctn( cos( tau * now ) );

    // Temperature variation
    const double mod_t( 0 ); // TODO: make this depend on latitude and altitude?
    const double current_t( base_temperature +
                            mod_t ); // Current baseline temperature. Degrees Celsius.
    const double seasonal_variation( ctn * -1 ); // Start and end at -1 going up to 1 in summer.
    const double season_atenuation( ctn / 2 + 1 ); // Harsh winter nights, hot summers.
    const double season_dispersion( pow( 2,
                                         ctn + 1 ) - 2.3 ); // Make summers peak faster and winters not perma-frozen.
    const double daily_variation( cos( tau * dayFraction - tau / 8 ) * -1 * season_atenuation / 2 +
                                  season_dispersion * -1 ); // Day-night temperature variation.

    T += current_t; // Add baseline to the noise.
    T += seasonal_variation * 8 * exp( -pow( current_t * 2.7 / 10 - 0.5,
                                       2 ) ); // Add season curve offset to account for the winter-summer difference in day-night difference.
    T += daily_variation * 8 * exp( -pow( current_t / 30,
                                          2 ) ); // Add daily variation scaled to the inverse of the current baseline. A very specific and finicky adjustment curve.
    T = T * 9 / 5 + 32; // Convert to imperial. =|

    // Humidity variation
    const double mod_h( 0 );
    const double current_h( base_humidity + mod_h );
    H = std::max( std::min( ( ctn / 10.0 + ( -pow( H, 2 ) * 3 + H2 ) ) * current_h / 2.0 + current_h,
                            100.0 ),
                  0.0 ); // Humidity stays mostly at the mean level, but has low peaks rarely. It's a percentage.

    // Pressure variation
    P += seasonal_variation * 20 +
         base_pressure; // Pressure is mostly random, but a bit higher on summer and lower on winter. In millibars.

    // Wind power
    W = std::max( 0, 1020 - ( int )P );

    // Acid rains
    const double acid_content = base_acid * A;
    bool acid = acid_content >= 1.0;

    return w_point {T, H, P, W, acid};
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
        } else if( r > WEATHER_THUNDER ) { // @todo: that is always false!
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
    //@todo: this is wrong. weather_generator does not have such a constructor
    // weather_generator WEATHERGEN(0); // Seeds the weather object.
    // WEATHERGEN.test_weather(); // Runs this test.
    std::ofstream testfile;
    testfile.open( "weather.output", std::ofstream::trunc );
    testfile << "turn,temperature(F),humidity(%),pressure(mB)" << std::endl;

    const time_point begin = calendar::turn;
    const time_point end = begin + 2 * calendar::year_length();
    for( time_point i = begin; i < end; i += 200_turns ) {
        //@todo: a new random value for each call to get_weather? Is this really intended?
        w_point w = get_weather( tripoint( 0, 0, 0 ), to_turn<int>( i ), rand() );
        testfile << to_turn<int>( i ) << "," << w.temperature << "," << w.humidity << "," << w.pressure <<
                 std::endl;
    }
}

weather_generator weather_generator::load( JsonObject &jo )
{
    weather_generator ret;
    ret.base_temperature = jo.get_float( "base_temperature", 6.5 );
    ret.base_humidity = jo.get_float( "base_humidity", 66.0 );
    ret.base_pressure = jo.get_float( "base_pressure", 1015.0 );
    ret.base_acid = jo.get_float( "base_acid", 1015.0 );
    return ret;
}
