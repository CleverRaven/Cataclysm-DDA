#pragma once
#ifndef CATA_SRC_WEATHER_H
#define CATA_SRC_WEATHER_H

#include "calendar.h"
#include "catacharset.h"
#include "color.h"
#include "coordinates.h"
#include "optional.h"
#include "pimpl.h"
#include "point.h"
#include "type_id.h"
#include "weather_gen.h"
#include "weather_type.h"

class JsonIn;
class JsonOut;
class translation;

/**
 * @name BODYTEMP
 * Body temperature.
 * Body temperature is measured on a scale of 0u to 10000u, where 10u = 0.02C and 5000u is 37C
 * Outdoor temperature uses similar numbers, but on a different scale: 2200u = 22C, where 10u = 0.1C.
 * Most values can be changed with no impact on calculations.
 * Maximum heat cannot pass 15000u, otherwise the player will vomit to death.
 */
///@{
//!< More aggressive cold effects.
static constexpr int BODYTEMP_FREEZING = 500;
//!< This value means frostbite occurs at the warmest temperature of 1C. If changed, the temp_conv calculation should be reexamined.
static constexpr int BODYTEMP_VERY_COLD = 2000;
//!< Frostbite timer will not improve while below this point.
static constexpr int BODYTEMP_COLD = 3500;
//!< Do not change this value, it is an arbitrary anchor on which other calculations are made.
static constexpr int BODYTEMP_NORM = 5000;
//!< Level 1 hotness.
static constexpr int BODYTEMP_HOT = 6500;
//!< Level 2 hotness.
static constexpr int BODYTEMP_VERY_HOT = 8000;
//!< Level 3 hotness.
static constexpr int BODYTEMP_SCORCHING = 9500;

//!< Additional Threshold before speed is impacted by heat.
static constexpr int BODYTEMP_THRESHOLD = 500;
///@}

#include <cstdint>
#include <iosfwd>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

class Character;
class Creature;
class item;
struct rl_vec2d;
struct trap;

double precip_mm_per_hour( precip_class p );
void handle_weather_effects( const weather_type_id &w );

/**
 * Weather drawing tracking.
 * Used for redrawing the view coordinates overwritten by the previous frame's animation bits (raindrops, snowflakes, etc,) and to draw this frame's weather animation.
 * @see game::get_player_input
 */
struct weather_printable {
    //!< Weather type in use.
    weather_type_id wtype;
    //!< Coordinates targeted for droplets.
    std::vector<std::pair<int, int> > vdrops;
    //!< Color to draw glyph this animation frame.
    nc_color colGlyph;
    //!< Glyph to draw this animation frame.
    uint32_t cGlyph;
    std::string get_symbol() const {
        return utf32_to_utf8( cGlyph );
    }
};

struct weather_sum {
    int rain_amount = 0;
    int acid_amount = 0;
    float sunlight = 0.0f;
    float radiant_exposure = 0.0f; // J/m2
    int wind_amount = 0;
};
bool is_creature_outside( const Creature &target );
void wet_character( Character &target, int amount );
weather_type_id get_bad_weather();
std::string get_shortdirstring( int angle );

std::string get_dirstring( int angle );

std::string weather_forecast( const point_abs_sm &abs_sm_pos );

// Returns input value (in Fahrenheit) converted to whatever temperature scale set in options.
//
// If scale is Celsius:    temperature(100) will return "37C"
// If scale is Fahrenheit: temperature(100) will return "100F"
//
// Use the decimals parameter to set number of decimal places returned in string.
std::string print_temperature( units::temperature temperature, int decimals = 0 );
std::string print_humidity( double humidity, int decimals = 0 );
std::string print_pressure( double pressure, int decimals = 0 );

// Returns temperature delta caused by windchill at given temperature, humidity and wind
units::temperature get_local_windchill( units::temperature temperature, double humidity,
                                        double wind_mph );

int get_local_humidity( double humidity, const weather_type_id &weather, bool sheltered = false );

// Returns windspeed (mph) after being modified by local cover
int get_local_windpower( int windpower, const oter_id &omter, const tripoint &location,
                         const int &winddirection,
                         bool sheltered = false );
weather_sum sum_conditions( const time_point &start,
                            const time_point &end,
                            const tripoint &location );

/**
 * @param it The container item which is to be filled.
 * @param pos The absolute position of the funnel (in the map square system, the one used
 * by the @ref map, but absolute).
 * @param tr The funnel (trap which acts as a funnel).
 */
void retroactively_fill_from_funnel( item &it, const trap &tr, const time_point &start,
                                     const time_point &end, const tripoint &pos );

double funnel_charges_per_turn( double surface_area_mm2, double rain_depth_mm_per_hour );

rl_vec2d convert_wind_to_coord( int angle );

std::string get_wind_arrow( int );

std::string get_wind_desc( double );

nc_color get_wind_color( double );

/**
 * Is it warm enough to plant seeds?
 *
 * The first overload is in map-square coords, the second for larger scale
 * queries.
 */
bool warm_enough_to_plant( const tripoint &pos );
bool warm_enough_to_plant( const tripoint_abs_omt &pos );

bool is_wind_blocker( const tripoint &location );

weather_type_id current_weather( const tripoint &location,
                                 const time_point &t = calendar::turn );

void glare( const weather_type_id &w );
/**
 * Amount of sunlight incident at the ground, taking weather and time of day
 * into account.
 */
float incident_sunlight( const weather_type_id &wtype,
                         const time_point &t = calendar::turn );

/* Amount of irradiance (W/m2) at ground after weather modifications */
float incident_sun_irradiance( const weather_type_id &wtype,
                               const time_point &t = calendar::turn );

void weather_sound( const translation &sound_message, const std::string &sound_effect );

class weather_manager
{
    public:
        weather_manager();
        const weather_generator &get_cur_weather_gen() const;
        // Updates the temperature and weather patten
        void update_weather();
        // The air temperature
        units::temperature temperature = 0_K;
        bool lightning_active = false;
        // Weather pattern
        weather_type_id weather_id = WEATHER_NULL;
        int winddirection = 0;
        int windspeed = 0;
        // Cached weather data
        pimpl<w_point> weather_precise;
        cata::optional<int> wind_direction_override;
        cata::optional<int> windspeed_override;
        weather_type_id weather_override;
        // not only sets nextweather, but updates weather as well
        void set_nextweather( time_point t );
        // The time at which weather will shift next.
        time_point nextweather;
        /** temperature cache, cleared every turn, sparse map of map tripoints to temperatures */
        std::unordered_map< tripoint, units::temperature > temperature_cache;
        // Returns outdoor or indoor temperature of given location
        units::temperature get_temperature( const tripoint &location );
        // Returns outdoor or indoor temperature of given location
        units::temperature get_temperature( const tripoint_abs_omt &location ) const;
        void clear_temp_cache();
        static void unserialize_all( JsonIn &jsin );
};

weather_manager &get_weather();
const weather_manager &get_weather_const();

#endif // CATA_SRC_WEATHER_H
