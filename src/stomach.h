#pragma once
#ifndef CATA_SRC_STOMACH_H
#define CATA_SRC_STOMACH_H

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "calendar.h"
#include "type_id.h"
#include "units.h"

class Character;
class JsonObject;
class JsonOut;
struct needs_rates;

namespace vitamin_units
{
using mass = units::quantity<int, units::mass_in_microgram_tag>;

constexpr mass microgram = units::quantity<int, units::mass_in_microgram_tag>( 1, {} );
constexpr mass milligram = units::quantity<int, units::mass_in_microgram_tag>( 1000, {} );
constexpr mass gram = units::quantity<int, units::mass_in_microgram_tag>( 1000000, {} );
const std::vector<std::pair<std::string, mass>> mass_units = { {
        { "ug", microgram },
        { "μg", microgram },
        { "mcg", microgram },
        { "mg", milligram },
        { "g", gram }
    }
};
} // namespace vitamin_units

// Separate struct for nutrients so that we can easily perform arithmetic on
// them
struct nutrients {
        friend struct islot_comestible; //this is for loading vitamins from JSON only
        /** amount of calories (1/1000s of kcal) this food has */
        int64_t calories = 0;

        /** Replace the values here with the minimum (or maximum) of themselves and the corresponding
         * values taken from r. */
        void min_in_place( const nutrients &r );
        void max_in_place( const nutrients &r );

        // vitamin -> how many vitamin units of it are included
        std::map<vitamin_id, int> vitamins() const;

        // For vitamins that support units::mass quantities
        // If finalized == true, these will instantly convert to int,
        // so make sure finalized = false if you call these before vitamins are loaded
        void set_vitamin( const vitamin_id &, vitamin_units::mass mass );
        void add_vitamin( const vitamin_id &, vitamin_units::mass mass );

        void set_vitamin( const vitamin_id &, int units );
        void add_vitamin( const vitamin_id &, int units );

        // Remove a vitamin completely from the data structure
        void remove_vitamin( const vitamin_id & );

        int get_vitamin( const vitamin_id & ) const;
        int kcal() const;

        void finalize_vitamins();

        bool operator==( const nutrients &r ) const;
        bool operator!=( const nutrients &r ) const {
            return !( *this == r );
        }

        nutrients operator-();

        nutrients &operator+=( const nutrients &r );
        nutrients &operator-=( const nutrients &r );
        nutrients &operator*=( int r );
        nutrients &operator*=( double r );
        nutrients &operator/=( int r );

        friend nutrients operator*( nutrients l, int r ) {
            l *= r;
            return l;
        }

        friend nutrients operator/( nutrients l, int r ) {
            l /= r;
            return l;
        }

        // All vitamins are in vitamin units, not units::mass (e.g. all JSON has been loaded)
        // defaults to true because this is only false when nutrients are loaded from JSON,
        // where it is set explicitly to false
        bool finalized = true; // NOLINT(cata-serialize)

        void serialize( JsonOut & ) const;
        void deserialize( const JsonObject &jo );

        void clear_vitamins();

    private:
        /** vitamins potentially provided by this comestible (if any) */
        std::map<vitamin_id, std::variant<int, vitamin_units::mass>>
                vitamins_; // NOLINT(cata-serialize)
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
    units::volume solids = 0_ml;
    units::volume water = 0_ml;
    float percent_kcal = 0.0f;
    // calories, or 1/1000s of kcals
    int min_calories = 0;
    float percent_vitamin = 0.0f;
    int min_vitamin = 0;
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
        // These functions need a ref to the stomach's owner because capacity() does
        bool would_be_engorged_with( const Character &owner, units::volume intake,
                                     bool calorie_deficit ) const;
        bool would_be_full_with( const Character &owner, units::volume intake, bool calorie_deficit ) const;
        // how much stomach capacity you have left before you puke from stuffing your gob
        units::volume stomach_remaining( const Character &owner ) const;
        // how much volume is in the stomach_contents
        units::volume contains() const;

        int get_calories() const;
        units::volume get_water() const;

        // changes calorie amount
        void mod_calories( int kcal );

        // changes calorie amount based on old nutr value
        void mod_nutr( int nutr );
        // changes water amount in stomach
        // overflow draws from player thirst
        void mod_water( const units::volume &h2o );
        // changes water amount in stomach converted from quench value
        // TODO: Move to mL values of water
        void mod_quench( int quench );
        // adds volume to your stomach
        void mod_contents( const units::volume &vol );

        // how long has it been since i ate?
        // only really relevant for player::stomach
        time_duration time_since_ate() const;
        // update last_ate to calendar::turn
        void ate();

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &jo );

    private:

        // If true, this object represents a stomach; if false, this object represents guts.
        bool stomach = false; // NOLINT(cata-serialize)

        // nutrients (calories and vitamins)
        nutrients nutr;
        // volume of water in stomach_contents
        units::volume water = 0_ml;
        /**
        * this is the maximum volume without modifiers such as mutations
        * in order to get the maximum volume with all modifiers properly,
        * call stomach_capacity()
        */
        units::volume max_volume = 0_ml;
        // volume of food in stomach_contents
        units::volume contents = 0_ml;
        // when did this stomach_contents call stomach_contents::ingest()
        time_point last_ate = calendar::turn_zero;

        // Gets the rates at which this stomach will digest things.
        stomach_digest_rates get_digest_rates( const needs_rates &metabolic_rates,
                                               const Character &owner ) const;

};

#endif // CATA_SRC_STOMACH_H
