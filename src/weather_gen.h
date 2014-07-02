#ifndef _WEATHER_GEN_H_
#define _WEATHER_GEN_H_

#include "PerlinNoise/PerlinNoise.hpp"
#include "calendar.h"

struct w_point {
    double temperature;
    double humidity;
    double pressure;
};

class weather_generator
{
//friend class calendar;
    unsigned SEED;
    const int year_length;
    // Data source: Wolfram Alpha
    static constexpr double base_t = 6.5; // Average temperature of New England
    static constexpr double base_h = 66.0; // Average humidity
    static constexpr double base_p = 1015.0; // Average atmospheric pressure
    PerlinNoise Temperature;
    PerlinNoise Humidity;
    PerlinNoise Pressure;
public:
    weather_generator(unsigned seed);
    w_point get_weather(double x, double y, calendar t);
    void test_weather();
};
#endif
