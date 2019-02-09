#pragma once
#ifndef ACTIVITY_TYPE_H
#define ACTIVITY_TYPE_H

#include "string_id.h"

class activity_type;
class JsonObject;
class player;
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
        std::string stop_phrase_ = "THIS IS A BUG";
        bool suspendable_ = true;
        based_on_type based_on_ = based_on_type::SPEED;
        bool no_resume_ = false;
        bool refuel_fires = false;

    public:
        const activity_id &id() const {
            return id_;
        }
        bool rooted() const {
            return rooted_;
        }
        bool suspendable() const {
            return suspendable_;
        }
        std::string stop_phrase() const {
            return stop_phrase_;
        }
        based_on_type based_on() const {
            return based_on_;
        }
        bool no_resume() const {
            return no_resume_;
        }
        /**
         * If true, player will refuel one adjacent fire if there is firewood spot adjacent.
         */
        bool will_refuel_fires() const {
            return refuel_fires;
        }

        void call_do_turn( player_activity *, player * ) const;
        /** Returns whether it had a finish function or not */
        bool call_finish( player_activity *, player * ) const;

        /** JSON stuff */
        static void load( JsonObject &jo );
        static void check_consistency();
        static void reset();
};

#endif
