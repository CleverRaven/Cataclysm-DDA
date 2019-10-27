#pragma once

#include <map>
#include <utility>

#include "units.h"
#include "calendar.h"
#include "type_id.h"

struct needs_rates;
class player;
class JsonIn;
class JsonOut;
class item;

// how much the stomach_contents passes
// based on 30 minute increments
struct stomach_pass_rates {
    units::volume min_vol;
    float percent_vol;
    int min_kcal;
    float percent_kcal;
    int min_vit;
    float percent_vit;
};

// how much a stomach_contents can absorb
// based on 30 minute increments
struct stomach_absorb_rates {
    float percent_kcal;
    int min_kcal;
    std::map<vitamin_id, float> percent_vitamin;
    std::map<vitamin_id, int> min_vitamin;
    float percent_vitamin_default;
    int min_vitamin_default;
};

// an abstract of food that has been eaten.
class stomach_contents
{
    public:
        stomach_contents();
        stomach_contents( units::volume max_volume );

        // empties the stomach_contents of all of its contents
        void bowel_movement();
        // empties contents equal to amount as ratio
        // amount ranges from 0 to 1
        void bowel_movement( const stomach_pass_rates &rates );
        // moves @rates contents to other stomach_contents
        // amount ranges from 0 to 1
        void bowel_movement( const stomach_pass_rates &rates, stomach_contents &move_to );

        // turns an item into stomach contents
        // will still add contents if past maximum volume.
        void ingest( player &p, item &food, int charges );

        // calculates max volume for a stomach_contents
        units::volume capacity() const;
        // how much stomach capacity you have left before you puke from stuffing your gob
        units::volume stomach_remaining() const;
        // how much volume is in the stomach_contents
        units::volume contains() const;

        // calculates and sets absorbed kcal and vitamins
        void calculate_absorbed( stomach_absorb_rates rates );

        // gets the rates of passing contents out of stomach_contents if true and guts if false
        stomach_pass_rates get_pass_rates( bool stomach );
        // gets the absorption rates for kcal and vitamins
        // stomach == true, guts == false
        stomach_absorb_rates get_absorb_rates( bool stomach, const needs_rates &metabolic_rates );

        int get_calories() const;
        int get_calories_absorbed() const;
        units::volume get_water() const;

        // changes calorie amount
        void mod_calories( int calories );

        // changes calorie amount based on old nutr value
        void mod_nutr( int nutr );
        // changes water amount in stomach
        // overflow draws from player thirst
        void mod_water( units::volume h2o );
        // changes water amount in stomach converted from quench value
        // @TODO: Move to mL values of water
        void mod_quench( int quench );
        // adds volume to your stomach
        void mod_contents( units::volume vol );

        void absorb_water( player &p, units::volume amount );

        // moves absorbed nutrients to the player for use
        // returns true if any calories are absorbed
        // does not empty absorbed calories
        bool store_absorbed( player &p );

        // how long has it been since i ate?
        // only really relevant for player::stomach
        time_duration time_since_ate() const;
        // update last_ate to calendar::turn
        void ate();

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &json );

    private:

        // vitamins in stomach_contents
        std::map<vitamin_id, int> vitamins;
        // vitamins to be absorbed in stomach_contents
        std::map<vitamin_id, int> vitamins_absorbed;
        // number of calories in stomach_contents
        int calories = 0;
        // number of calories to be absorbed in stomach_contents
        int calories_absorbed = 0;
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

};
