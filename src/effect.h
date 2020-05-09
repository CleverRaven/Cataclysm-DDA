#pragma once
#ifndef CATA_SRC_EFFECT_H
#define CATA_SRC_EFFECT_H

#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "bodypart.h"
#include "calendar.h"
#include "hash_utils.h"
#include "translations.h"
#include "type_id.h"

class player;

enum game_message_type : int;
class JsonIn;
class JsonObject;
class JsonOut;

/** Handles the large variety of weed messages. */
void weed_msg( player &p );

enum effect_rating {
    e_good,     // The effect is good for the one who has it.
    e_neutral,  // There is no effect or the effect is very nominal. This is the default.
    e_bad,      // The effect is bad for the one who has it.
    e_mixed     // The effect has good and bad parts to the one who has it.
};

class effect_type
{
        friend void load_effect_type( const JsonObject &jo );
        friend class effect;
    public:
        effect_type() = default;

        efftype_id id;

        /** Returns if an effect is good or bad for message display. */
        effect_rating get_rating() const;

        /** Returns true if there is a listed name in the JSON entry for each intensity from
         *  1 to max_intensity. */
        bool use_name_ints() const;
        /** Returns true if there is a listed description in the JSON entry for each intensity
         *  from 1 to max_intensity with the matching reduced value. */
        bool use_desc_ints( bool reduced ) const;

        /** Returns the appropriate game_message_type when a new effect is obtained. This is equal to
         *  an effect's "rating" value. */
        game_message_type gain_game_message_type() const;
        /** Returns the appropriate game_message_type when an effect is lost. This is opposite to
         *  an effect's "rating" value. */
        game_message_type lose_game_message_type() const;

        /** Returns the message displayed when a new effect is obtained. */
        std::string get_apply_message() const;
        /** Returns the memorial log added when a new effect is obtained. */
        std::string get_apply_memorial_log() const;
        /** Returns the message displayed when an effect is removed. */
        std::string get_remove_message() const;
        /** Returns the memorial log added when an effect is removed. */
        std::string get_remove_memorial_log() const;

        /** Returns true if an effect will only target main body parts (i.e., those with HP). */
        bool get_main_parts() const;

        bool is_show_in_info() const;

        /** Loading helper functions */
        bool load_mod_data( const JsonObject &jo, const std::string &member );
        bool load_miss_msgs( const JsonObject &jo, const std::string &member );
        bool load_decay_msgs( const JsonObject &jo, const std::string &member );

        /** Registers the effect in the global map */
        static void register_ma_buff_effect( const effect_type &eff );

    protected:
        int max_intensity = 0;
        int max_effective_intensity = 0;
        time_duration max_duration = 0_turns;

        int dur_add_perc = 0;
        int int_add_val = 0;

        int int_decay_step = 0;
        int int_decay_tick = 0 ;
        time_duration int_dur_factor = 0_turns;

        std::set<std::string> flags;

        bool main_parts_only = false;

        // Determines if effect should be shown in description.
        bool show_in_info = false;

        std::vector<trait_id> resist_traits;
        std::vector<efftype_id> resist_effects;
        std::vector<efftype_id> removes_effects;
        std::vector<efftype_id> blocks_effects;

        std::vector<std::pair<std::string, int>> miss_msgs;

        bool pain_sizing = false;
        bool hurt_sizing = false;
        bool harmful_cough = false;
        // TODO: Once addictions are JSON-ized it should be trivial to convert this to a
        // "generic" addiction reduces value
        bool pkill_addict_reduces = false;
        // This flag is hard-coded for specific IDs now
        // It needs to be set for monster::move_effects
        bool impairs_movement = false;

        std::vector<translation> name;
        std::string speed_mod_name;
        std::vector<std::string> desc;
        std::vector<std::string> reduced_desc;
        bool part_descs = false;

        std::vector<std::pair<std::string, game_message_type>> decay_msgs;

        effect_rating rating = effect_rating::e_neutral;

        std::string apply_message;
        std::string apply_memorial_log;
        std::string remove_message;
        std::string remove_memorial_log;

        /** Key tuple order is:("base_mods"/"scaling_mods", reduced: bool, type of mod: "STR", desired argument: "tick") */
        std::unordered_map <
        std::tuple<std::string, bool, std::string, std::string>, double, cata::tuple_hash > mod_data;
};

class effect
{
    public:
        effect() : eff_type( nullptr ), duration( 0_turns ), bp( num_bp ),
            permanent( false ), intensity( 1 ), start_time( calendar::turn_zero ) {
        }
        effect( const effect_type *peff_type, const time_duration &dur, body_part part,
                bool perm, int nintensity, const time_point &nstart_time ) :
            eff_type( peff_type ), duration( dur ), bp( part ),
            permanent( perm ), intensity( nintensity ), start_time( nstart_time ) {
        }
        effect( const effect & ) = default;
        effect &operator=( const effect & ) = default;

        /** Returns true if the effect is the result of `effect()`, ie. an effect that doesn't exist. */
        bool is_null() const;

        /** Dummy used for "reference to effect()" */
        static effect null_effect;

        /** Returns the name displayed in the player status window. */
        std::string disp_name() const;
        /** Returns the description displayed in the player status window. */
        std::string disp_desc( bool reduced = false ) const;
        /** Returns the short description as set in json. */
        std::string disp_short_desc( bool reduced = false ) const;
        /** Returns true if a description will be formatted as "Your" + body_part + description. */
        bool use_part_descs() const;

        /** Returns the effect's matching effect_type. */
        const effect_type *get_effect_type() const;

        /** Decays effect durations, pushing their id and bp's back to rem_ids and rem_bps for removal later
         *  if their duration is <= 0. This is called in the middle of a loop through all effects, which is
         *  why we aren't allowed to remove the effects here. */
        void decay( std::vector<efftype_id> &rem_ids, std::vector<body_part> &rem_bps,
                    const time_point &time, bool player );

        /** Returns the remaining duration of an effect. */
        time_duration get_duration() const;
        /** Returns the maximum duration of an effect. */
        time_duration get_max_duration() const;
        /** Sets the duration, capping at max_duration if it exists. */
        void set_duration( const time_duration &dur, bool alert = false );
        /** Mods the duration, capping at max_duration if it exists. */
        void mod_duration( const time_duration &dur, bool alert = false );
        /** Multiplies the duration, capping at max_duration if it exists. */
        void mult_duration( double dur, bool alert = false );

        /** Returns the turn the effect was applied. */
        time_point get_start_time() const;

        /** Returns the targeted body_part of the effect. This is num_bp for untargeted effects. */
        body_part get_bp() const;
        /** Sets the targeted body_part of an effect. */
        void set_bp( body_part part );

        /** Returns true if an effect is permanent, i.e. it's duration does not decrease over time. */
        bool is_permanent() const;
        /** Makes an effect permanent. Note: This pauses the duration, but does not otherwise change it. */
        void pause_effect();
        /** Makes an effect not permanent. Note: This un-pauses the duration, but does not otherwise change it. */
        void unpause_effect();

        /** Returns the intensity of an effect. */
        int get_intensity() const;
        /** Returns the maximum intensity of an effect. */
        int get_max_intensity() const;

        /**
         * Sets intensity of effect capped by range [1..max_intensity]
         * @param val Value to set intensity to
         * @param alert whether decay messages should be displayed
         * @return new intensity of the effect after val subjected to above cap
         */
        int set_intensity( int val, bool alert = false );

        /**
         * Modify intensity of effect capped by range [1..max_intensity]
         * @param mod Amount to increase current intensity by
         * @param alert whether decay messages should be displayed
         * @return new intensity of the effect after modification and capping
         */
        int mod_intensity( int mod, bool alert = false );

        /** Returns the string id of the resist trait to be used in has_trait("id"). */
        const std::vector<trait_id> &get_resist_traits() const;
        /** Returns the string id of the resist effect to be used in has_effect("id"). */
        const std::vector<efftype_id> &get_resist_effects() const;
        /** Returns the string ids of the effects removed by this effect to be used in remove_effect("id"). */
        const std::vector<efftype_id> &get_removes_effects() const;
        /** Returns the string ids of the effects blocked by this effect to be used in add_effect("id"). */
        std::vector<efftype_id> get_blocks_effects() const;

        /** Returns the matching modifier type from an effect, used for getting actual effect effects. */
        int get_mod( std::string arg, bool reduced = false ) const;
        /** Returns the average return of get_mod for a modifier type. Used in effect description displays. */
        int get_avg_mod( std::string arg, bool reduced = false ) const;
        /** Returns the amount of a modifier type applied when a new effect is first added. */
        int get_amount( std::string arg, bool reduced = false ) const;
        /** Returns the minimum value of a modifier type that get_mod() and get_amount() will push the player to. */
        int get_min_val( std::string arg, bool reduced = false ) const;
        /** Returns the maximum value of a modifier type that get_mod() and get_amount() will push the player to. */
        int get_max_val( std::string arg, bool reduced = false ) const;
        /** Returns true if the given modifier type's trigger chance is affected by size mutations. */
        bool get_sizing( const std::string &arg ) const;
        /** Returns the approximate percentage chance of a modifier type activating on any given tick, used for descriptions. */
        double get_percentage( std::string arg, int val, bool reduced = false ) const;
        /** Checks to see if a given modifier type can activate, and performs any rolls required to do so. mod is a direct
         *  multiplier on the overall chance of a modifier type activating. */
        bool activated( const time_point &when, std::string arg, int val,
                        bool reduced = false, double mod = 1 ) const;

        /** Check if the effect has the specified flag */
        bool has_flag( const std::string &flag ) const;

        /** Returns the modifier caused by addictions. Currently only handles painkiller addictions. */
        double get_addict_mod( const std::string &arg, int addict_level ) const;
        /** Returns true if the coughs caused by an effect can harm the player directly. */
        bool get_harmful_cough() const;
        /** Returns the percentage value by further applications of existing effects' duration is multiplied by. */
        int get_dur_add_perc() const;
        /** Returns the number of turns it takes for the intensity to fall by 1 or 0 if intensity isn't based on duration. */
        time_duration get_int_dur_factor() const;
        /** Returns the amount an already existing effect intensity is modified by further applications of the same effect. */
        int get_int_add_val() const;

        /** Returns a vector of the miss message messages and chances for use in add_miss_reason() while the effect is in effect. */
        std::vector<std::pair<std::string, int>> get_miss_msgs() const;

        /** Returns the value used for display on the speed modifier window in the player status menu. */
        std::string get_speed_name() const;

        /** Returns if the effect is supposed to be handed in Creature::movement */
        bool impairs_movement() const;

        /** Returns the effect's matching effect_type id. */
        const efftype_id &get_id() const {
            return eff_type->id;
        }

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );

    protected:
        const effect_type *eff_type;
        time_duration duration;
        body_part bp;
        bool permanent;
        int intensity;
        time_point start_time;

};

void load_effect_type( const JsonObject &jo );
void reset_effect_types();

std::string texitify_base_healing_power( int power );
std::string texitify_healing_power( int power );

// Inheritance here allows forward declaration of the map in class Creature.
// Storing body_part as an int to make things easier for hash and JSON
class effects_map : public
    std::unordered_map<efftype_id, std::unordered_map<body_part, effect, std::hash<int>>>
{
};

#endif // CATA_SRC_EFFECT_H
