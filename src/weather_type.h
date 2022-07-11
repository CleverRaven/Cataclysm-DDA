#pragma once
#ifndef CATA_SRC_WEATHER_TYPE_H
#define CATA_SRC_WEATHER_TYPE_H

#include <climits>
#include <cstdint>
#include <iosfwd>
#include <new>
#include <string>
#include <vector>

#include "calendar.h"
#include "catacharset.h"
#include "color.h"
#include "damage.h"
#include "optional.h"
#include "translations.h"
#include "type_id.h"

class JsonObject;
template <typename E> struct enum_traits;
struct dialogue;
template<typename T>
class generic_factory;

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

enum weather_sound_category : int {
    silent,
    drizzle,
    rainy,
    thunder,
    flurries,
    snowstorm,
    snow,
    portal_storm,
    clear,
    sunny,
    cloudy,
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
    float factor = 0.0f;
    nc_color color = c_white;
    uint32_t symbol = NULL_UNICODE;
    std::string get_symbol() const {
        return utf32_to_utf8( symbol );
    }
};

struct weather_type {
    public:
        friend class generic_factory<weather_type>;
        bool was_loaded = false;
        weather_type_id id;
        std::vector<std::pair<weather_type_id, mod_id>> src;
        // UI name of weather type.
        translation name;
        // UI color of weather type.
        nc_color color = c_white;
        // Map color of weather type.
        nc_color map_color = c_white;
        // Map glyph of weather type.
        uint32_t symbol = PERCENT_SIGN_UNICODE;
        // Penalty to ranged attacks.
        int ranged_penalty = 0;
        // Penalty to per-square visibility, applied in transparency map.
        float sight_penalty = 0.0f;
        // Modification to ambient light.
        int light_modifier = 0;
        // Multiplier to radiation from Sun.
        float sun_multiplier = 1.f;
        // Sound attenuation of a given weather type.
        int sound_attn = 0;
        // If true, our activity gets interrupted.
        bool dangerous = false;
        // Amount of associated precipitation.
        precip_class precip = precip_class::none;
        // Whether said precipitation falls as rain.
        bool rains = false;
        // Whether said precipitation is acidic.
        bool acidic = false;
        // string for tiles animation
        std::string tiles_animation;
        // Information for weather animations
        weather_animation_t weather_animation;
        // if playing sound effects what to use
        weather_sound_category sound_category = weather_sound_category::silent;
        // strength of the sun
        sun_intensity_type sun_intensity = sun_intensity_type::none;
        // when this weather should happen
        std::function<bool( const dialogue & )> condition;
        std::vector<weather_type_id> required_weathers;
        time_duration duration_min = 0_turns;
        time_duration duration_max = 0_turns;
        void load( const JsonObject &jo, const std::string &src );
        void finalize();
        void check() const;
        std::string get_symbol() const {
            return utf32_to_utf8( symbol );
        }
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
