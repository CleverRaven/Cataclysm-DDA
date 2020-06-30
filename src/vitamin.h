#pragma once
#ifndef CATA_SRC_VITAMIN_H
#define CATA_SRC_VITAMIN_H

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "calendar.h"
#include "translations.h"
#include "type_id.h"

class JsonObject;
template <typename T> struct enum_traits;

enum class vitamin_type : int {
    VITAMIN,
    TOXIN,
    DRUG,
    COUNTER,
    num_vitamin_types
};

template<>
struct enum_traits<vitamin_type> {
    static constexpr auto last = vitamin_type::num_vitamin_types;
};

class vitamin
{
    public:
        vitamin() : id_( vitamin_id( "null" ) ), rate_( 1_hours ) {}

        const vitamin_id &id() const {
            return id_;
        }

        const vitamin_type &type() const {
            return type_;
        }

        bool is_null() const {
            return id_ == vitamin_id( "null" );
        }

        std::string name() const {
            return name_.translated();
        }

        bool has_flag( const std::string &flag ) const {
            return flags_.count( flag ) > 0;
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
        static void load_vitamin( const JsonObject &jo );

        /** Get all currently loaded vitamins */
        static const std::map<vitamin_id, vitamin> &all();

        /** Check consistency of all loaded vitamins */
        static void check_consistency();

        /** Clear all loaded vitamins (invalidating any pointers) */
        static void reset();

    private:
        vitamin_id id_;
        vitamin_type type_ = vitamin_type::num_vitamin_types;
        translation name_;
        efftype_id deficiency_;
        efftype_id excess_;
        int min_ = 0;
        int max_ = 0;
        time_duration rate_ = 0_turns;
        std::vector<std::pair<int, int>> disease_;
        std::vector<std::pair<int, int>> disease_excess_;
        std::set<std::string> flags_;
};

#endif // CATA_SRC_VITAMIN_H
