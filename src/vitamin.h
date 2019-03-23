#pragma once
#ifndef VITAMIN_H
#define VITAMIN_H

#include <map>
#include <utility>
#include <vector>

#include "calendar.h"
#include "string_id.h"

class JsonObject;
class vitamin;
using vitamin_id = string_id<vitamin>;
class effect_type;
using efftype_id = string_id<effect_type>;

class vitamin
{
    public:
        vitamin() : id_( vitamin_id( "null" ) ), rate_( 1_hours ) {}

        const vitamin_id &id() const {
            return id_;
        }

        bool is_null() const {
            return id_ == vitamin_id( "null" );
        }

        const std::string &name() const {
            return name_;
        }

        /** Disease effect with increasing intensity proportional to vitamin deficiency */
        const efftype_id &deficiency() const {
            return deficiency_;
        }

        /** Disease effect with increasing intensity proportional to vitamin excess */
        const efftype_id &excess() const {
            return excess_;
        }

        /** Lower bound for deficiency of this vitamin */
        int min() const {
            return min_;
        }

        /** Upper bound for any accumulation of this vitamin */
        int max() const {
            return max_;
        }

        /**
         * Usage rate of vitamin (time to consume unit)
         * Lower bound is zero whereby vitamin is not required (but may still accumulate)
         */
        time_duration rate() const {
            return rate_;
        }

        /** Get intensity of deficiency or zero if not deficient for specified qty */
        int severity( int qty ) const;

        /** Load vitamin from JSON definition */
        static void load_vitamin( JsonObject &jo );

        /** Get all currently loaded vitamins */
        static const std::map<vitamin_id, vitamin> &all();

        /** Check consistency of all loaded vitamins */
        static void check_consistency();

        /** Clear all loaded vitamins (invalidating any pointers) */
        static void reset();

    private:
        vitamin_id id_;
        std::string name_;
        efftype_id deficiency_;
        efftype_id excess_;
        int min_;
        int max_;
        time_duration rate_;
        std::vector<std::pair<int, int>> disease_;
};

#endif
