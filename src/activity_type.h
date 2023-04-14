#pragma once
#ifndef CATA_SRC_ACTIVITY_TYPE_H
#define CATA_SRC_ACTIVITY_TYPE_H

#include <iosfwd>

#include "game_constants.h"
#include "string_id.h"
#include "translations.h"
#include "type_id.h"

class JsonObject;
class activity_type;
class Character;
class player_activity;

using activity_id = string_id<activity_type>;

/** @relates string_id */
template<>
const activity_type &string_id<activity_type>::obj() const;

enum class based_on_type : int {
    TIME = 0,
    SPEED,
    NEITHER
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
        bool suspendable_ = true;
        based_on_type based_on_ = based_on_type::SPEED;
        bool no_resume_ = false;
        bool multi_activity_ = false;
        bool refuel_fires = false;
        bool auto_needs = false;
        float activity_level = NO_EXERCISE;
    public:
        effect_on_condition_id completion_EOC;
        effect_on_condition_id do_turn_EOC;

        const activity_id &id() const {
            return id_;
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
        bool suspendable() const {
            return suspendable_;
        }
        std::string stop_phrase() const;
        const translation &verb() const {
            return verb_;
        }
        based_on_type based_on() const {
            return based_on_;
        }
        bool no_resume() const {
            return no_resume_;
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
