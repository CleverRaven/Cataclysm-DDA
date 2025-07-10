#pragma once
#ifndef CATA_SRC_ACTIVITY_TYPE_H
#define CATA_SRC_ACTIVITY_TYPE_H

#include <set>
#include <string>

#include "game_constants.h"
#include "translation.h"
#include "type_id.h"

class Character;
class JsonObject;
class activity_type;
class player_activity;
enum class distraction_type : int;
template <typename T> struct enum_traits;

/** @relates string_id */
template<>
const activity_type &string_id<activity_type>::obj() const;

enum class based_on_type : int {
    TIME = 0,
    SPEED,
    NEITHER,
    last,
};

template<>
struct enum_traits<based_on_type> {
    static constexpr based_on_type last = based_on_type::last;
};

/** A class that stores constant information that doesn't differ between activities of the same type */
class activity_type
{
    private:
        activity_id id_;
        bool rooted_ = false;
        translation verb_ = to_translation( "THIS IS A BUG" );
        bool interruptable_ = true;
        bool interruptable_with_kb_ = true;
        based_on_type based_on_ = based_on_type::SPEED;
        bool can_resume_ = true;
        bool multi_activity_ = false;
        bool refuel_fires = false;
        bool auto_needs = false;
        float activity_level = NO_EXERCISE;
        std::set<distraction_type> default_ignored_distractions_;
    public:
        effect_on_condition_id completion_EOC;
        effect_on_condition_id do_turn_EOC;

        const activity_id &id() const {
            return id_;
        }
        const std::set<distraction_type> &default_ignored_distractions() const {
            return default_ignored_distractions_;
        }
        bool rooted() const {
            return rooted_;
        }
        bool interruptable() const {
            return interruptable_;
        }
        bool interruptable_with_kb() const {
            return interruptable_with_kb_;
        }
        std::string stop_phrase() const;
        const translation &verb() const {
            return verb_;
        }
        based_on_type based_on() const {
            return based_on_;
        }
        bool can_resume() const {
            return can_resume_;
        }
        bool multi_activity() const {
            return multi_activity_;
        }
        /**
         * If true, player will refuel one adjacent fire if there is firewood spot adjacent.
         */
        bool will_refuel_fires() const {
            return refuel_fires;
        }
        /**
         * If true, player will automatically consume from relevant auto-eat/drink zones during activity
         */
        bool valid_auto_needs() const {
            return auto_needs;
        }
        float exertion_level() const {
            return activity_level;
        }
        void call_do_turn( player_activity *, Character * ) const;
        /** Returns whether it had a finish function or not */
        bool call_finish( player_activity *, Character * ) const;

        /** JSON stuff */
        static void load( const JsonObject &jo );
        static void check_consistency();
        static void reset();
};

#endif // CATA_SRC_ACTIVITY_TYPE_H
