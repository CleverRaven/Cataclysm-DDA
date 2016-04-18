#ifndef VITAMIN_H
#define VITAMIN_H

#include "json.h"
#include "effect.h"

class vitamin;
using vitamin_id = string_id<vitamin>;

class vitamin
{
    public:
        vitamin() : id_( vitamin_id( "null" ) ) {}

        const vitamin_id &id() const {
            return id_;
        }

        bool is_null() const {
            return id_ == vitamin_id( "null" );
        }

        const std::string &name() const {
            return name_;
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
         * Usage rate of vitamin (minutes to consume unit)
         * Lower bound is zero whereby vitamin is not required (but may still accumulate)
         * If unspecified in JSON a default value of 60 minutes is used
         */
        int rate() const {
            return rate_;
        }

        /** Get applicable status effect (if any) at @ref level */
        const efftype_id &effect( int level ) const;

        /** Load vitamin from JSON definition */
        static void load_vitamin( JsonObject &jo );

        /** Get all currently loaded vitamins */
        static const std::map<vitamin_id, vitamin> &all();

        /** Clear all loaded vitamins (invalidating any pointers) */
        static void reset();

    private:
        vitamin_id id_;
        std::string name_;
        int min_;
        int max_;
        int rate_;
        std::vector<std::pair<efftype_id, int>> deficiency_;
        std::vector<std::pair<efftype_id, int>> excess_;
};

#endif
