#include "weather_gen.h"
#include "weather.h"
#include "enums.h"
#include "calendar.h"
#include "simplexnoise.h"

#include <cmath>
#include <fstream>

namespace {
constexpr double tau = 6.28318530717958647693; // aka 2PI aka 2 * std::acos(-1);
// Data source: Wolfram Alpha
constexpr double base_t = 6.5; // Average temperature of New England
constexpr double base_h = 66.0; // Average humidity
constexpr double base_p = 1015.0; // Average atmospheric pressure
} //namespace

weather_generator::weather_generator()
{
    debug_weather = WEATHER_NULL;
}

w_point weather_generator::get_weather(const tripoint &location, const calendar &t) const
{
    return get_weather( point( location.x, location.y ), t );
}

w_point weather_generator::get_weather(const point &location, const calendar &t) const
{
    const double x(location.x / 2000.0);// Integer x position / widening factor of the Perlin function.
    const double y(location.y / 2000.0);// Integer y position / widening factor of the Perlin function.
    // Leaving these in just in case something ELSE goes wrong--KA101
//    int initial_season(0);
//    if(ACTIVE_WORLD_OPTIONS["INITIAL_SEASON"].getValue() == "spring") {
//        initial_season = 1;
//    } else if(ACTIVE_WORLD_OPTIONS["INITIAL_SEASON"].getValue() == "summer") {
//        initial_season = 2;
//    } else if(ACTIVE_WORLD_OPTIONS["INITIAL_SEASON"].getValue() == "autumn") {
//        initial_season = 3;
//    }
    const double z( double( t.get_turn() + DAYS(t.season_length()) ) / 2000.0); // Integer turn / widening factor of the Perlin function.

    const double dayFraction((double)t.minutes_past_midnight() / 1440);

    //limit the random seed during noise calculation, a large value flattens the noise generator to zero
    //Windows has a rand limit of 32768, other operating systems can have higher limits
    const unsigned modSEED = SEED % 32768;

    // Noise factors
    double T(raw_noise_4d(x, y, z, modSEED) * 4.0);
    double H(raw_noise_4d(x, y, z / 5, modSEED + 101));
    double H2(raw_noise_4d(x, y, z, modSEED + 151) / 4);
    double P(raw_noise_4d(x, y, z / 3, modSEED + 211) * 70);
    double W;

    const double now( double( t.turn_of_year() + DAYS(t.season_length()) / 2 ) / double(t.year_turns()) ); // [0,1)
    const double ctn(cos(tau * now));

    // Temperature variation
    const double mod_t(0); // TODO: make this depend on latitude and altitude?
    const double current_t(base_t + mod_t); // Current baseline temperature. Degrees Celsius.
    const double seasonal_variation(ctn * -1); // Start and end at -1 going up to 1 in summer.
    const double season_atenuation(ctn / 2 + 1); // Harsh winter nights, hot summers.
    const double season_dispersion(pow(2,
                                       ctn + 1) - 2.3); // Make summers peak faster and winters not perma-frozen.
    const double daily_variation(cos( tau * dayFraction - tau / 8 ) * -1 * season_atenuation / 2 +
                                 season_dispersion * -1); // Day-night temperature variation.

    T += current_t; // Add baseline to the noise.
    T += seasonal_variation * 8 * exp(-pow(current_t * 2.7 / 10 - 0.5,
                                            2)); // Add season curve offset to account for the winter-summer difference in day-night difference.
    T += daily_variation * 8 * exp(-pow(current_t / 30,
                                         2)); // Add daily variation scaled to the inverse of the current baseline. A very specific and finicky adjustment curve.
    T = T * 9 / 5 + 32; // Convert to imperial. =|

    // Humidity variation
    const double mod_h(0);
    const double current_h(base_h + mod_h);
    H = std::max(std::min((ctn / 10.0 + (-pow(H, 2) * 3 + H2)) * current_h / 2.0 + current_h, 100.0),
                 0.0); // Humidity stays mostly at the mean level, but has low peaks rarely. It's a percentage.

    // Pressure variation
    P += seasonal_variation * 20 +
         base_p; // Pressure is mostly random, but a bit higher on summer and lower on winter. In millibars.

    // Wind power
    W = std::max(0, 1020 - (int)P);

    return w_point {T, H, P, W, false};
}

weather_type weather_generator::get_weather_conditions(const point &location, const calendar &t) const
{
    if( debug_weather != WEATHER_NULL ) {
        // Debug mode weather forcing
        return debug_weather;
    }

    w_point w(get_weather(location, t));
    weather_type wt = get_weather_conditions(w);
    // Make sure we don't say it's sunny at night! =P
    if (wt == WEATHER_SUNNY && t.is_night()) { return WEATHER_CLEAR; }
    return wt;
}

weather_type weather_generator::get_weather_conditions(const w_point &w) const
{
    if( debug_weather != WEATHER_NULL ) {
        // Debug mode weather forcing
        return debug_weather;
    }

    weather_type r(WEATHER_CLEAR);
    if (w.pressure > 1030 && w.humidity < 70) {
        r = WEATHER_SUNNY;
    }
    if (w.pressure < 1030 && w.humidity > 40) {
        r = WEATHER_CLOUDY;
    }
    if (r == WEATHER_CLOUDY && (w.humidity > 60 || w.pressure < 1010)) {
        r = WEATHER_DRIZZLE;
    }
    if (r == WEATHER_DRIZZLE && (w.humidity > 70 || w.pressure < 1000)) {
        r = WEATHER_RAINY;
    }
    if (r == WEATHER_RAINY && w.pressure < 985) {
        r = WEATHER_THUNDER;
    }
    if (r == WEATHER_THUNDER && w.pressure < 970) {
        r = WEATHER_LIGHTNING;
    }

    if (w.temperature <= 32) {
        if (r == WEATHER_DRIZZLE) {
            r = WEATHER_FLURRIES;
        } else if (r > WEATHER_DRIZZLE) {
            r = WEATHER_SNOW;
        } else if (r > WEATHER_THUNDER) {
            r = WEATHER_SNOWSTORM;
        }
    }

    if (r == WEATHER_DRIZZLE && w.acidic) {
        r = WEATHER_ACID_DRIZZLE;
    }
    if (r > WEATHER_DRIZZLE && w.acidic) {
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

    int season_length = calendar::turn.season_length();
    int day = calendar::turn.day_of_year();
    int hour = calendar::turn.hours();
    int water_temperature = 0;

    if (season_length == 0) season_length = 1;

    // Temperature varies between 33.8F and 75.2F depending on the time of year. Day = 0 corresponds to the start of spring.
    int annual_mean_water_temperature = 54.5 + 20.7 * sin(tau * (day - season_length*0.5) / (season_length*4.0));
    // Temperature vareis between +2F and -2F depending on the time of day. Hour = 0 corresponds to midnight.
    int daily_water_temperature_varaition = 2.0 + 2.0 * sin(tau * (hour - 6.0) / 24.0);

    water_temperature = annual_mean_water_temperature + daily_water_temperature_varaition;

    return water_temperature;
}

void weather_generator::test_weather() const
{
    // Outputs a Cata year's worth of weather data to a csv file.
    // Usage:
    // weather_generator WEATHERGEN(0); // Seeds the weather object.
    // WEATHERGEN.test_weather(); // Runs this test.
    std::ofstream testfile;
    testfile.open("weather.output", std::ofstream::trunc);
    testfile << "turn,temperature(F),humidity(%),pressure(mB)" << std::endl;

    for (calendar i(calendar::turn); i.get_turn() < calendar::turn + 14400 * 2 * calendar::turn.year_length(); i+=200) {
        w_point w = get_weather(point(0, 0), i);
        testfile << i.get_turn() << "," << w.temperature << "," << w.humidity << "," << w.pressure << std::endl;
    }
}
