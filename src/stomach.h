#pragma once
#ifndef CATA_SRC_STOMACH_H
#define CATA_SRC_STOMACH_H

#include <map>

#include "calendar.h"
#include "type_id.h"

class JsonIn;
class JsonOut;
struct needs_rates;

// Separate struct for nutrients so that we can easily perform arithmetic on
// them
struct nutrients {
    /** amount of kcal this food has */
    int kcal = 0;

    /** vitamins potentially provided by this comestible (if any) */
    std::map<vitamin_id, int> vitamins;

    /** Replace the values here with the minimum (or maximum) of themselves and the corresponding
     * values taken from r. */
    void min_in_place( const nutrients &r );
    void max_in_place( const nutrients &r );

    int get_vitamin( const vitamin_id & ) const;

    bool operator==( const nutrients &r ) const;
    bool operator!=( const nutrients &r ) const {
        return !( *this == r );
    }

    nutrients &operator+=( const nutrients &r );
    nutrients &operator-=( const nutrients &r );
    nutrients &operator*=( int r );
    nutrients &operator/=( int r );

    friend nutrients operator*( nutrients l, int r ) {
        l *= r;
        return l;
    }

    friend nutrients operator/( nutrients l, int r ) {
        l /= r;
        return l;
    }
};

// Contains all information that can pass out of (or into) a stomach
struct food_summary {
    nutrients nutr;
};

// how much a stomach_contents can digest
// based on 30 minute increments
struct stomach_digest_rates {
    float percent_kcal;
    int min_kcal;
    float percent_vitamin;
    int min_vitamin;
};

// an abstract of food that has been eaten.
class stomach_contents
{
    public:
        stomach_contents();

        /**
         * @brief Directly adds food to stomach contents.
         * Will still add contents if past maximum volume. Also updates last_ate to current turn.
         * @param ingested The food to be ingested
         */
        void ingest( const food_summary &ingested );

        /**
         * @brief Processes food and outputs nutrients that are finished processing
         * Metabolic rates are required because they determine the rate of absorption of
         * nutrients into the body.
         * @param metabolic_rates The metabolic rates of the owner of this stomach
         * @param five_mins Five-minute intervals passed since this method was last called
         */
        food_summary digest( const needs_rates &metabolic_rates, int five_mins );

        // Empties the stomach of all contents.
        void empty();

        int get_calories() const;

        // changes calorie amount
        void mod_calories( int calories );

        // how long has it been since i ate?
        // only really relevant for player::stomach
        time_duration time_since_ate() const;
        // update last_ate to calendar::turn
        void ate();

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &json );

    private:

        // nutrients (calories and vitamins)
        nutrients nutr;
        // when did this stomach_contents call stomach_contents::ingest()
        time_point last_ate;

        // Gets the rates at which this stomach will digest things.
        stomach_digest_rates get_digest_rates( const needs_rates &metabolic_rates );

};

#endif // CATA_SRC_STOMACH_H
