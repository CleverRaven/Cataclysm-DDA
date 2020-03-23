#pragma once

#include <map>

#include "type_id.h"
#include "units.h"

struct needs_rates;
class JsonIn;
class JsonOut;
class item;

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
    units::volume water;
    units::volume solids;
    nutrients nutr;
};

// how much a stomach_contents can digest
// based on 30 minute increments
struct stomach_digest_rates {
    units::volume solids;
    units::volume water;
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
         * @brief Construct a new stomach contents object
         * Stomachs always process their food in a few hours. Guts take significantly longer,
         * and their rate is dependent on the metabolic rate of their owner.
         * @param max_volume Base capacity, subject to modification, i.e. by traits
         * @param is_stomach If true, treated as stomach, if false, treated as guts
         */
        stomach_contents( units::volume max_volume, bool is_stomach );

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
         * All returned values are >= 0, with the exception of water, which
         * can be negative in some circumstances (i.e. after eating dry/salty food).
         * TODO: Update stomach capacity upon mutation changes, instead of calculating every time we
         * need it, so we can get rid of 'owner' parameter here.
         * @param owner The owner of this stomach
         * @param metabolic_rates The metabolic rates of the owner of this stomach
         * @param five_mins Five-minute intervals passed since this method was last called
         * @param half_hours Half-hour intervals passed since this method was last called
         * @return nutrients that are done processing in this stomach
         */
        food_summary digest( const Character &owner, const needs_rates &metabolic_rates,
                             int five_mins, int half_hours );

        // Empties the stomach of all contents.
        void empty();

        /**
         * @brief Calculates the capacity of this stomach.
         * This function needs a ref to the stomach's owner so it can account for relevant mutations.
         * TODO: JSONize stomach capacity multipliers.
         * @param owner This stomach's owner
         * @return This stomach's capacity, in units::volume
         */
        units::volume capacity( const Character &owner ) const;
        // how much stomach capacity you have left before you puke from stuffing your gob
        units::volume stomach_remaining( const Character &owner ) const;
        // how much volume is in the stomach_contents
        units::volume contains() const;

        int get_calories() const;
        units::volume get_water() const;

        // changes calorie amount
        void mod_calories( int calories );

        // changes calorie amount based on old nutr value
        void mod_nutr( int nutr );
        // changes water amount in stomach
        // overflow draws from player thirst
        void mod_water( units::volume h2o );
        // changes water amount in stomach converted from quench value
        // TODO: Move to mL values of water
        void mod_quench( int quench );
        // adds volume to your stomach
        void mod_contents( units::volume vol );

        // how long has it been since i ate?
        // only really relevant for player::stomach
        time_duration time_since_ate() const;
        // update last_ate to calendar::turn
        void ate();

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &json );

    private:

        // If true, this object represents a stomach; if false, this object represents guts.
        bool stomach;

        // nutrients (calories and vitamins)
        nutrients nutr;
        // volume of water in stomach_contents
        units::volume water;
        /**
        * this is the maximum volume without modifiers such as mutations
        * in order to get the maximum volume with all modifiers properly,
        * call stomach_capacity()
        */
        units::volume max_volume;
        // volume of food in stomach_contents
        units::volume contents;
        // when did this stomach_contents call stomach_contents::ingest()
        time_point last_ate;

        // Gets the rates at which this stomach will digest things.
        stomach_digest_rates get_digest_rates( const needs_rates &metabolic_rates,
                                               const Character &owner );

};
