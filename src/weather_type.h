#pragma once
#ifndef CATA_SRC_WEATHER_TYPE_H
#define CATA_SRC_WEATHER_TYPE_H

#include <algorithm>
#include <climits>
#include <string>
#include <vector>

#include "bodypart.h"
#include "calendar.h"
#include "color.h"
#include "damage.h"
#include "field.h"
#include "string_id.h"
#include "translations.h"
#include "type_id.h"

class JsonObject;
template <typename E> struct enum_traits;
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

enum class weather_sound_category : int {
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
    float factor = 0.0f;
    nc_color color = c_white;
    uint32_t symbol = NULL_UNICODE;
    std::string get_symbol() const {
        return utf32_to_utf8( symbol );
    }
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
    weather_time_requirement_type time = weather_time_requirement_type::both;
    std::vector<weather_type_id> required_weathers{};
    time_duration time_passed_min = 0_turns;
    time_duration time_passed_max = 0_turns;
    int one_in_chance = 0;
};

struct weather_field {
    field_type_str_id type;
    int intensity = 0;
    time_duration age = 0_turns;
    int radius = 0;
    bool outdoor_only = false;
};

struct spawn_type {
    mtype_id target;
    int target_range = 0;
    int hallucination_count = 0;
    int real_count = 0;
    int min_radius = 0;
    int max_radius = 0;
};

struct weather_effect {
    int one_in_chance = 0;
    time_duration time_between = 0_turns;
    translation message;
    bool must_be_outside = false;
    translation sound_message;
    std::string sound_effect;
    bool lightning = false;
    bool rain_proof = false;
    int pain = 0;
    int pain_max = 0;
    int wet = 0;
    int radiation = 0;
    int healthy = 0;
    efftype_id effect_id;
    time_duration effect_duration = 0_turns;
    trait_id trait_id_to_add;
    trait_id trait_id_to_remove;
    bodypart_str_id target_part;
    cata::optional<damage_instance> damage;
    std::vector<spawn_type> spawns;
    std::vector<weather_field> fields;
};

struct weather_type {
    public:
        friend class generic_factory<weather_type>;
        bool was_loaded = false;
        weather_type_id id;
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
        // vector for weather effects.
        std::vector<weather_effect> effects{};
        // string for tiles animation
        std::string tiles_animation;
        // Information for weather animations
        weather_animation_t weather_animation;
        // if playing sound effects what to use
        weather_sound_category sound_category = weather_sound_category::silent;
        // strength of the sun
        sun_intensity_type sun_intensity = sun_intensity_type::none;
        // when this weather should happen
        weather_requirements requirements;
        time_duration duration_min = 0_turns;
        time_duration duration_max = 0_turns;
        time_duration time_between_min = 0_turns;
        time_duration time_between_max = 0_turns;
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
