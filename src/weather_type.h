#pragma once
#ifndef CATA_SRC_WEATHER_TYPE_H
#define CATA_SRC_WEATHER_TYPE_H

#include <string>

#include "generic_factory.h"
#include "translations.h"
#include "type_id.h"

using weather_effect_fn = void ( * )( int intensity );

const weather_type_id WEATHER_NULL( "null" );
const weather_type_id WEATHER_CLEAR( "clear" );

enum class precip_class : int {
    none,
    very_light,
    light,
    heavy,
    last
};
template<>
struct enum_traits<precip_class> {
    static constexpr precip_class last = precip_class::last;
};

enum class sun_intensity_type : int {
    none,
    light,
    normal,
    high,
    last
};
template<>
struct enum_traits<sun_intensity_type > {
    static constexpr sun_intensity_type last = sun_intensity_type::last;
};

enum class weather_time_requirement_type : int {
    day,
    night,
    both,
    last
};
template<>
struct enum_traits<weather_time_requirement_type> {
    static constexpr weather_time_requirement_type last = weather_time_requirement_type::last;
};


enum weather_sound_category : int {
    silent,
    drizzle,
    rainy,
    thunder,
    flurries,
    snowstorm,
    snow,
    last
};

template<>
struct enum_traits<weather_sound_category> {
    static constexpr weather_sound_category last = weather_sound_category::last;
};

/**
 * Weather animation class.
 */
struct weather_animation_t {
    float    factor;
    nc_color color;
    char     glyph;
};

struct weather_requirements {
    int windpower_min = INT_MIN;
    int windpower_max = INT_MAX;
    int temperature_min = INT_MIN;
    int temperature_max = INT_MAX;
    int pressure_min = INT_MIN;
    int pressure_max = INT_MAX;
    int humidity_min = INT_MIN;
    int humidity_max = INT_MAX;
    bool humidity_and_pressure = true;
    bool acidic = false;
    weather_time_requirement_type time;
    std::vector<weather_type_id> required_weathers;
};

struct weather_type {
    public:
        friend class generic_factory<weather_type>;
        bool was_loaded = false;
        weather_type_id id;
        std::string name;             //!< UI name of weather type.
        nc_color color;               //!< UI color of weather type.
        nc_color map_color;           //!< Map color of weather type.
        char glyph;                   //!< Map glyph of weather type.
        int ranged_penalty;           //!< Penalty to ranged attacks.
        float sight_penalty;          //!< Penalty to per-square visibility, applied in transparency map.
        int light_modifier;           //!< Modification to ambient light.
        int sound_attn;               //!< Sound attenuation of a given weather type.
        bool dangerous;               //!< If true, our activity gets interrupted.
        precip_class precip;          //!< Amount of associated precipitation.
        bool rains;                   //!< Whether said precipitation falls as rain.
        bool acidic;                  //!< Whether said precipitation is acidic.
        std::vector < std::pair < weather_effect_fn, int >> effects;      //!< vector for weather effects.
        std::string tiles_animation;  //!< string for tiles animation
        weather_animation_t weather_animation; //!< Information for weather animations
        weather_sound_category sound_category; //!< if playing sound effects what to use
        sun_intensity_type sun_intensity; //!< strength of the sun
        weather_requirements requirements; //!< when this weather should happen

        void load( const JsonObject &jo, const std::string &src );
        void finalize();
        void check() const;
        weather_type() = default;
};
namespace weather_types
{
/** Get all currently loaded weather types */
const std::vector<weather_type> &get_all();
/** Finalize all loaded weather types */
void finalize_all();
/** Clear all loaded weather types (invalidating any pointers) */
void reset();
/** Load weather type from JSON definition */
void load( const JsonObject &jo, const std::string &src );
/** Checks all loaded from JSON are valid */
void check_consistency();
} // namespace weather_types
#endif // CATA_SRC_WEATHER_TYPE_H
