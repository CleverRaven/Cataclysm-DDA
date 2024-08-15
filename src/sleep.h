#pragma once
#ifndef CATA_SRC_SLEEP_H
#define CATA_SRC_SLEEP_H

#include <string>
#include <vector>

#include "calendar.h"
#include "enum_traits.h"
#include "enums.h"
#include "point.h"
#include "translation.h"

class Character;
class JsonObject;

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
        field,
        vehicle,
        character,
        trait,
        last
    };

    struct condition {
        category category;
        std::string id;
        std::string flag;
        int intensity = 1;
        bool active = false;
        bool invert = false;

        bool is_condition_true( const Character &guy, const tripoint &p ) const;

        void deserialize( const JsonObject &jo );
    };

    struct message {
        translation text;
        game_message_type type = game_message_type::m_neutral;

        void deserialize( const JsonObject &jo );
    };

    struct cache {
        comfort_data &data;
        int comfort;
        std::string sleep_aid;
        tripoint last_position;
        time_point last_time;
    };

    std::vector<condition> conditions;
    int comfort = COMFORT_NEUTRAL;
    bool add_human_comfort = false;
    bool add_sleep_aids = false;
    message msg_try;
    message msg_fall;

    static const comfort_data &human();

    bool are_conditions_true( const Character &guy, const tripoint &p ) const;

    void deserialize_comfort( const JsonObject &jo, bool was_loaded );
    void deserialize( const JsonObject &jo );
    bool was_loaded = false;
};

template<> struct enum_traits<comfort_data::category> {
    static constexpr comfort_data::category last = comfort_data::category::last;
};

#endif // CATA_SRC_SLEEP_H
