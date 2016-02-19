#ifndef WEATHER_GEN_H
#define WEATHER_GEN_H

struct point;
struct tripoint;
class calendar;
enum weather_type : int;

struct w_point {
    double temperature;
    double humidity;
    double pressure;
    double windpower;
    bool   acidic;
};

class weather_generator
{
    public:
        weather_generator();

        /**
         * Input should be an absolute position in the map square system (the one used
         * by the @ref map). You can use @ref map::getabs to get an absolute position from a
         * relative position (relative to the map you called getabs on).
         */
        w_point get_weather( const point &, const calendar & ) const;
        w_point get_weather( const tripoint &, const calendar & ) const;
        weather_type get_weather_conditions( const point &, const calendar & ) const;
        weather_type get_weather_conditions( const w_point & ) const;
        int get_water_temperature() const;
        void test_weather() const;

        void set_seed( unsigned seed ) {
            SEED = seed;
        }

        unsigned get_seed() const {
            return SEED;
        }

        /**
         * If set to anything but WEATHER_NULL, overrides all weather generation.
         */
        weather_type debug_weather;
    private:
        unsigned SEED;
};

#endif
