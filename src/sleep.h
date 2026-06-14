#pragma once
#ifndef CATA_SRC_SLEEP_H
#define CATA_SRC_SLEEP_H

#include <string>
#include <vector>

#include "calendar.h"
#include "coordinates.h"
#include "enums.h"
#include "point.h"
#include "translation.h"

class Character;
class JsonObject;
class item;
template <typename T> struct enum_traits;

/**
 * Information for evaluating the comfort of a location.
 *
 * @details
 * Some mutations allow mutants to sleep in locations that unmutated characters would find
 * uncomfortable. Comfort data provides alternative comfort values to locations that fulfill their
 * conditions, as well as specialized messages for falling asleep under those conditions.
 */
struct comfort_data {
    static const int COMFORT_IMPOSSIBLE = -999;
    static const int COMFORT_UNCOMFORTABLE = -7;
    static const int COMFORT_NEUTRAL = 0;
    static const int COMFORT_SLIGHTLY_COMFORTABLE = 3;
    static const int COMFORT_SLEEP_AID = 4;
    static const int COMFORT_COMFORTABLE = 5;
    static const int COMFORT_VERY_COMFORTABLE = 10;

    enum class category {
        terrain,
        furniture,
        trap,
        field,
        vehicle,
        character,
        trait,
        last
    };

    struct condition {
        category ccategory;
        std::string id;
        std::string flag;
        /** True if the given field's intensity is greater than or equal to this **/
        int intensity = 1;
        /** True if the given trait is actived **/
        bool active = false;
        /** If the truth value of the condition should be inverted **/
        bool invert = false;

        bool is_condition_true( const Character &guy, const tripoint_bub_ms &p ) const;
        void deserialize( const JsonObject &jo );
    };

    struct message {
        translation text;
        game_message_type type = game_message_type::m_neutral;

        void deserialize( const JsonObject &jo );
    };

    struct response {
        /** The comfort data that produced this response **/
        const comfort_data *data;
        int comfort;
        /** The name of a used sleep aid, if one exists **/
        std::string sleep_aid;
        tripoint_bub_ms last_position;
        time_point last_time;

        void add_try_msgs( const Character &guy ) const;
        void add_sleep_msgs( const Character &guy ) const;
    };

    std::vector<condition> conditions;
    /** If conditions should be ORed (true) or ANDed (false) together **/
    bool conditions_or = false;
    int base_comfort = COMFORT_NEUTRAL;
    /** If the human comfort of a location should be added to base comfort **/
    bool add_human_comfort = false;
    /** If human comfort should be used instead of base comfort when better **/
    bool use_better_comfort = false;
    /** If comfort from sleep aids should be added to base comfort **/
    bool add_sleep_aids = false;
    message msg_try;
    message msg_hint;
    message msg_sleep;

    /** The comfort data of an unmutated human **/
    static const comfort_data &human();
    /** The comfort of a location as provided by its furniture/traps/terrain **/
    static int human_comfort_at( const tripoint_bub_ms &p );
    /** If there is a sleep aid at a location. The sleep aid will be stored in `result` if it exists **/
    static bool try_get_sleep_aid_at( const tripoint_bub_ms &p, item &result );
    /** Deserializes an int or string to a comfort value (int) and stores it in `member` **/
    static void deserialize_comfort( const JsonObject &jo, bool was_loaded,
                                     const std::string &name, int &member );

    bool human_or_impossible() const;
    bool are_conditions_true( const Character &guy, const tripoint_bub_ms &p ) const;
    response get_comfort_at( const tripoint_bub_ms &p ) const;

    void deserialize( const JsonObject &jo );
    bool was_loaded = false;
};

template<> struct enum_traits<comfort_data::category> {
    static constexpr comfort_data::category last = comfort_data::category::last;
};

#endif // CATA_SRC_SLEEP_H
