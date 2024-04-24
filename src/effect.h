#pragma once
#ifndef CATA_SRC_EFFECT_H
#define CATA_SRC_EFFECT_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "calendar.h"
#include "color.h"
#include "effect_source.h"
#include "enums.h"
#include "event.h"
#include "flat_set.h"
#include "translation.h"
#include "type_id.h"

class Character;
class JsonObject;
class JsonOut;
class effect_type;

/** Handles the large variety of weed messages. */
void weed_msg( Character &p );

/** @relates string_id */
template<>
const effect_type &string_id<effect_type>::obj() const;

struct vitamin_rate_effect {
    std::vector<std::pair<int, int>> rate;
    std::vector<float> absorb_mult;
    std::vector<time_duration> tick;

    std::vector<std::pair<int, int>> red_rate;
    std::vector<float> red_absorb_mult;
    std::vector<time_duration> red_tick;

    vitamin_id vitamin;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo );
};

struct vitamin_applied_effect {
    std::optional<std::pair<int, int>> rate = std::nullopt;
    std::optional<time_duration> tick = std::nullopt;
    std::optional<float> absorb_mult = std::nullopt;
    vitamin_id vitamin;
};

enum class mod_type : uint8_t {
    BASE_MOD = 0,
    SCALING_MOD = 1,

    MAX,
};

using modifier_value_arr = std::array<double, static_cast<size_t>( mod_type::MAX )>;
extern modifier_value_arr default_modifier_values;

enum class mod_action : uint8_t {
    AMOUNT,
    CHANCE_BOT,
    CHANCE_TOP,
    MAX,
    MAX_VAL,
    MIN,
    MIN_VAL,
    TICK,
};

// Limb score interactions
struct limb_score_effect {
    // Score id to affect
    limb_score_id score_id;
    // Multiplier to apply when not resisted
    float mod;
    float red_mod;
    float scaling;
    float red_scaling;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo );
};

struct effect_dur_mod {
    efftype_id effect_id;
    float modifier;
    bool same_bp;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo );
};

class effect_type
{
        friend void load_effect_type( const JsonObject &jo );
        friend class effect;
    public:
        enum class memorial_gender : int {
            male,
            female,
        };

        effect_type() = default;

        efftype_id id;

        /** Returns if an effect is good or bad for message display. */
        game_message_type get_rating( int intensity = 1 ) const;

        /** Returns true if there is a listed name in the JSON entry for each intensity from
         *  1 to max_intensity. */
        bool use_name_ints() const;
        /** Returns true if there is a listed description in the JSON entry for each intensity
         *  from 1 to max_intensity with the matching reduced value. */
        bool use_desc_ints( bool reduced ) const;

        /** Returns the appropriate game_message_type when an effect is lost. This is opposite to
         *  an effect's "rating" value. */
        game_message_type lose_game_message_type( int intensity = 1 ) const;

        // adds a message to the log for applying an effect
        void add_apply_msg( int intensity ) const;

        /** Returns the memorial log added when a new effect is obtained. */
        std::string get_apply_memorial_log( memorial_gender gender ) const;
        /** Returns the message displayed when an effect is removed. */
        std::string get_remove_message() const;
        /** Returns the memorial log added when an effect is removed. */
        std::string get_remove_memorial_log( memorial_gender gender ) const;
        /** Returns the effect's description displayed when character conducts blood analysis. */
        std::string get_blood_analysis_description() const;

        /** Returns true if an effect will only target main body parts (i.e., those with HP). */
        bool get_main_parts() const;

        double get_mod_value( const std::string &type, mod_action action, uint8_t reduction_level,
                              int intensity ) const;

        bool is_show_in_info() const;

        /** Loading helper functions */
        void load_mod_data( const JsonObject &jo );
        bool load_miss_msgs( const JsonObject &jo, std::string_view member );
        bool load_decay_msgs( const JsonObject &jo, std::string_view member );
        bool load_apply_msgs( const JsonObject &jo, std::string_view member );

        /** Verifies data is accurate */
        static void check_consistency();
        void verify() const;

        /** Registers the effect in the global map */
        static void register_ma_buff_effect( const effect_type &eff );

        /** Check if the effect type has the specified flag */
        bool has_flag( const flag_id &flag ) const;

        const time_duration &intensity_duration() const {
            return int_dur_factor;
        }
        std::vector<enchantment_id> enchantments;
        cata::flat_set<json_character_flag> immune_flags;
        cata::flat_set<json_character_flag> immune_bp_flags;
    protected:
        uint32_t get_effect_modifier_key( mod_action action, uint8_t reduction_level ) const {
            return static_cast<uint8_t>( action ) << 0 |
                   reduction_level << 8;
        }

        void extract_effect(
            const std::array<std::optional<JsonObject>, 2> &j,
            const std::string &effect_name,
            const std::vector<std::pair<std::string, mod_action>> &action_keys );

        int max_intensity = 0;
        int max_effective_intensity = 0;
        time_duration max_duration = 365_days;

        int dur_add_perc = 0;
        int int_add_val = 0;

        int int_decay_step = 0;
        int int_decay_tick = 0 ;
        time_duration int_dur_factor = 0_turns;
        bool int_decay_remove = false;

        std::set<flag_id> flags;

        bool main_parts_only = false;

        // Determines if effect should be shown in description.
        bool show_in_info = false;

        // Determines if effect should show intensity value next to its name in EFFECTS tab.
        bool show_intensity = false;

        std::vector<trait_id> resist_traits;
        std::vector<efftype_id> resist_effects;
        std::vector<efftype_id> removes_effects;
        std::vector<efftype_id> blocks_effects;

        std::vector<std::pair<translation, int>> miss_msgs;

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
        translation speed_mod_name;
        std::vector<translation> desc;
        std::vector<translation> reduced_desc;
        bool part_descs = false;

        std::vector<std::pair<translation, game_message_type>> decay_msgs;

        std::vector<std::pair<translation, game_message_type>> apply_msgs;

        std::string apply_memorial_log;
        translation remove_message;
        std::string remove_memorial_log;

        translation blood_analysis_description;

        translation death_msg;
        std::optional<event_type> death_event;

        std::unordered_map<std::string, std::unordered_map<uint32_t, modifier_value_arr>> mod_data;
        std::vector<vitamin_rate_effect> vitamin_data;
        std::vector<limb_score_effect> limb_score_data;
        std::vector<effect_dur_mod> effect_dur_scaling;
        std::vector<std::pair<int, int>> kill_chance;
        std::vector<std::pair<int, int>> red_kill_chance;
};

class effect;

// Inheritance here allows forward declaration of the map in class Creature.
// Storing body_part as an int_id to make things easier for hash and JSON
class effects_map : public
    std::map<efftype_id, std::map<bodypart_id, effect>>
{
};

class effect
{
    public:
        effect() : eff_type( nullptr ), duration( 0_turns ), bp( bodypart_str_id::NULL_ID() ),
            permanent( false ), intensity( 1 ), start_time( calendar::turn_zero ),
            source( effect_source::empty() ) {
        }
        explicit effect( const effect_type *peff_type ) : eff_type( peff_type ), duration( 0_turns ),
            bp( bodypart_str_id::NULL_ID() ),
            permanent( false ), intensity( 1 ), start_time( calendar::turn_zero ),
            source( effect_source::empty() ) {
        }
        effect( const effect_source &source, const effect_type *peff_type, const time_duration &dur,
                bodypart_str_id part, bool perm, int nintensity, const time_point &nstart_time ) :
            eff_type( peff_type ), duration( dur ), bp( part ),
            permanent( perm ), intensity( nintensity ), start_time( nstart_time ),
            source( source ) {
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
        void decay( std::vector<efftype_id> &rem_ids, std::vector<bodypart_id> &rem_bps,
                    const time_point &time, bool player, const effects_map &eff_map = effects_map() );

        /** Returns the remaining duration of an effect. */
        time_duration get_duration() const;
        /** Returns the maximum duration of an effect. */
        time_duration get_max_duration() const;
        /** Sets the duration, capping at max duration. */
        void set_duration( const time_duration &dur, bool alert = false );
        /** Mods the duration, capping at max_duration. */
        void mod_duration( const time_duration &dur, bool alert = false );
        /** Multiplies the duration, capping at max_duration. */
        void mult_duration( double dur, bool alert = false );

        std::vector<vitamin_applied_effect> vit_effects( bool reduced ) const;

        /** Returns the turn the effect was applied. */
        time_point get_start_time() const;

        /** Returns the targeted body_part of the effect. This is bp_null for untargeted effects. */
        bodypart_id get_bp() const;
        /** Sets the targeted body_part of an effect. */
        void set_bp( const bodypart_str_id &part );

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
        /** Returns the maximum effective intensity of an effect. */
        int get_max_effective_intensity() const;
        /** Returns the current effect intensity, capped to max_effective_intensity. */
        int get_effective_intensity() const;

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
        int get_mod( const std::string &arg, bool reduced = false ) const;
        /** Returns the average return of get_mod for a modifier type. Used in effect description displays. */
        int get_avg_mod( const std::string &arg, bool reduced = false ) const;
        /** Returns the amount of a modifier type applied when a new effect is first added. */
        int get_amount( const std::string &arg, bool reduced = false ) const;
        /** Returns the minimum value of a modifier type that get_mod() and get_amount() will push the player to. */
        int get_min_val( const std::string &arg, bool reduced = false ) const;
        /** Returns the maximum value of a modifier type that get_mod() and get_amount() will push the player to. */
        int get_max_val( const std::string &arg, bool reduced = false ) const;
        /** Returns true if the given modifier type's trigger chance is affected by size mutations. */
        bool get_sizing( const std::string &arg ) const;
        /** Returns the approximate percentage chance of a modifier type activating on any given tick, used for descriptions. */
        double get_percentage( const std::string &arg, int val, bool reduced = false ) const;
        /** Checks to see if a given modifier type can activate, and performs any rolls required to do so. mod is a direct
         *  multiplier on the overall chance of a modifier type activating. */
        bool activated( const time_point &when, const std::string &arg, int val,
                        bool reduced = false, double mod = 1 ) const;

        /** Check if the effect has the specified flag */
        bool has_flag( const flag_id &flag ) const;

        // Extract limb score modifiers for descriptions
        std::vector<limb_score_effect> get_limb_score_data() const;

        std::vector<effect_dur_mod> get_effect_dur_scaling() const;

        bool kill_roll( bool reduced ) const;
        std::string get_death_message() const;
        event_type death_event() const;

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
        /** Returns the step of intensity decay */
        int get_int_decay_step() const;
        /** Returns the number of ticks between intensity changes */
        int get_int_decay_tick() const;
        /** Returns if the effect is not protected from intensity decay-based removal */
        bool get_int_decay_remove() const;

        /** Returns a vector of the miss message messages and chances for use in add_miss_reason() while the effect is in effect. */
        const std::vector<std::pair<translation, int>> &get_miss_msgs() const;

        /** Returns the value used for display on the speed modifier window in the player status menu. */
        std::string get_speed_name() const;

        /** Returns if the effect is supposed to be handed in Creature::movement */
        bool impairs_movement() const;

        float get_limb_score_mod( const limb_score_id &score, bool reduced = false ) const;

        /** Returns the effect's matching effect_type id. */
        const efftype_id &get_id() const {
            return eff_type->id;
        }

        const effect_source &get_source() const;

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &jo );

    protected:
        const effect_type *eff_type;
        time_duration duration;
        bodypart_str_id bp;
        bool permanent;
        int intensity;
        time_point start_time;
        effect_source source;

};

void load_effect_type( const JsonObject &jo );
void reset_effect_types();
const std::map<efftype_id, effect_type> &get_effect_types();

std::string texitify_base_healing_power( int power );
std::string texitify_healing_power( int power );
std::string texitify_bandage_power( int power );
nc_color colorize_bleeding_intensity( int intensity );

#endif // CATA_SRC_EFFECT_H
