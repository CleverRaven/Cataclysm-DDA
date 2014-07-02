#include "weather_gen.h"
#include "options.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <fstream>

const double pi = std::acos(-1);
const double tau = pi * 2.0;

weather_generator::weather_generator(unsigned seed) : SEED(seed), Temperature(SEED), Humidity(SEED + 101), Pressure(SEED + 211) { }

w_point weather_generator::get_weather(double x, double y, calendar t) {
    const double z = (double) t.get_turn() / 2000.0; // Integer turn / widening factor of the Perlin function.
    const double dayFraction((double)t.minutes_past_midnight() / 1440);

    // Noise factors
    double T(Temperature.noise(x, y, z) * 8.0);
    double H(Humidity.noise(x, y, z / 5));
    double H2(Humidity.noise(x, y, z) / 4);
    double P(Pressure.noise(x, y, z / 3) * 70);

    // temperature variation
    const double now((double)t.turn_of_year() / (double)calendar::year_turns()); // [0,1)
    const double ctn(cos(tau * now));
    const double mod_t = 0; // TODO: make this depend on latitude and altitude?
    const double current_t = this->base_t + mod_t; // Current baseline temperature. Degrees Celsius.
    const double seasonal_variation = ctn * -1; // Start and end at -1 going up to 1 in summer.
    const double season_atenuation = ctn / 2 + 1; // Harsh winter nights, hot summers.
    const double season_dispersion = pow(2, ctn * -1 + 1) - 2.3; // Make summers peak faster and winters not perma-frozen.
    const double daily_variation = cos( tau * dayFraction ) * season_atenuation + season_dispersion; // Day-night temperature variation.

    T += current_t; // Add baseline to the noise.

    const double D(seasonal_variation * 12 * exp(-pow(current_t * 2.7 / 20 - 0.5, 2)));
    T += D; // Add season curve offset to account for the winter-summer difference in day-night difference.

    T += daily_variation * 10 * exp(-pow(current_t / 30, 2)); //((4000 / (current_t + 115) - 24) + seasonal_variation); // Add daily variation scaled to the inverse of the current baseline. A very specific and finicky adjustment curve.

    // humidity variation
    const double mod_h = 0;
    const double current_h = this->base_h + mod_h;
    H = std::max(std::min((ctn / 10.0 + (-pow(H, 2)*3 + H2)) * current_h/2.0 + current_h, 100.0), 0.0); // Humidity stays mostly at the mean level, but has low peaks rarely. It's a percentage.

    // pressure variation
    const double mod_p = 0;
    const double current_p = this->base_p + mod_p;
    P += seasonal_variation * 20 + current_p; // Pressure is mostly random, but a bit higher on summer and lower on winter. In millibars.

    return w_point(T, H, P);
}

void weather_generator::test_weather() {
    // Outputs a Cata year's worth of weather data to a csv file.
    // Usage:
    // weather_generator WEATHERGEN(0); // Seeds the weather object.
    // WEATHERGEN.test_weather(); // Runs this test.
    std::ofstream testfile;
    std::ostringstream ss;
    testfile.open("weather.output", std::ofstream::trunc);
    testfile << "turn,temperature(C),humidity(%),pressure(mB)\n";
    for (calendar i(0); i.get_turn() < 14400 * calendar::year_length(); i+=200) {
//    for (calendar i(0); i.get_turn() < 1000; i.increment()) {
        ss.str("");
        w_point w = get_weather(0.5,0.5,i);
        ss << i.get_turn() << "," << w.temperature << "," << w.humidity << "," << w.pressure;
        testfile << std::string( ss.str() ) << "\n";
    }
    testfile.close();
}
