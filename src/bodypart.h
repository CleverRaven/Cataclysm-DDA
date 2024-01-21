#pragma once
#ifndef CATA_SRC_BODYPART_H
#define CATA_SRC_BODYPART_H

#include <array>
#include <climits>
#include <cstddef>
#include <initializer_list>
#include <iosfwd>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "damage.h"
#include "enums.h"
#include "flat_set.h"
#include "int_id.h"
#include "mod_tracker.h"
#include "string_id.h"
#include "translations.h"
#include "type_id.h"
#include "subbodypart.h"
#include "localized_comparator.h"
#include "type_id.h"
#include "weather.h"

class JsonObject;
class JsonOut;
class JsonValue;
struct body_part_type;
template <typename E> struct enum_traits;

using bodypart_str_id = string_id<body_part_type>;
using bodypart_id = int_id<body_part_type>;

extern const bodypart_str_id body_part_bp_null;
extern const bodypart_str_id body_part_head;
extern const bodypart_str_id body_part_eyes;
extern const bodypart_str_id body_part_mouth;
extern const bodypart_str_id body_part_torso;
extern const bodypart_str_id body_part_arm_l;
extern const bodypart_str_id body_part_arm_r;
extern const bodypart_str_id body_part_hand_l;
extern const bodypart_str_id body_part_hand_r;
extern const bodypart_str_id body_part_leg_l;
extern const bodypart_str_id body_part_foot_l;
extern const bodypart_str_id body_part_leg_r;
extern const bodypart_str_id body_part_foot_r;

extern const sub_bodypart_str_id sub_body_part_sub_limb_debug;

// The order is important ; pldata.h has to be in the same order
enum body_part : int {
    bp_torso = 0,
    bp_head,
    bp_eyes,
    bp_mouth,
    bp_arm_l,
    bp_arm_r,
    bp_hand_l,
    bp_hand_r,
    bp_leg_l,
    bp_leg_r,
    bp_foot_l,
    bp_foot_r,
    num_bp
};

template<>
struct enum_traits<body_part> {
    static constexpr body_part last = body_part::num_bp;
};

enum class side : int;

// Drench cache
enum water_tolerance {
    WT_IGNORED = 0,
    WT_NEUTRAL,
    WT_GOOD,
    NUM_WATER_TOLERANCE
};

/**
 * Contains all valid @ref body_part values in the order they are
 * defined in. Use this to iterate over them.
 */
constexpr std::array<body_part, 12> all_body_parts = {{
        bp_torso, bp_head, bp_eyes, bp_mouth,
        bp_arm_l, bp_arm_r, bp_hand_l, bp_hand_r,
        bp_leg_l, bp_leg_r, bp_foot_l, bp_foot_r
    }
};

struct stat_hp_mods {

    float str_mod = 3.0f;
    float dex_mod = 0.0f;
    float int_mod = 0.0f;
    float per_mod = 0.0f;

    float health_mod = 0.0f;

    bool was_loaded = false;
    void load( const JsonObject &jsobj );
    void deserialize( const JsonObject &jo );
};

struct limb_score {
    public:
        static void load_limb_scores( const JsonObject &jo, const std::string &src );
        static void reset();
        void load( const JsonObject &jo, std::string_view src );
        static const std::vector<limb_score> &get_all();

        const limb_score_id &getId() const {
            return id;
        }
        const translation &name() const {
            return _name;
        }
        bool affected_by_wounds() const {
            return wound_affect;
        }
        bool affected_by_encumb() const {
            return encumb_affect;
        }
    private:
        limb_score_id id;
        std::vector<std::pair<limb_score_id, mod_id>> src;
        translation _name;
        bool wound_affect = true;
        bool encumb_affect = true;
        bool was_loaded = false;
        friend class generic_factory<limb_score>;
        friend struct mod_tracker;
};

struct bp_limb_score {
    float score = 0.0f;
    float max = 0.0f;
};

struct bp_onhit_effect {
    // ID of the effect to apply
    efftype_id id;
    // Apply the effect to the given bodypart, or to the whole character?
    bool global = false;
    // Type of damage that causes the effect - NONE always applies
    damage_type_id dtype = damage_type_id::NULL_ID();
    // Percent of the limb's max HP required for the effect to trigger (or absolute DMG for minor limbs)
    int dmg_threshold = 100;
    // Percent HP / absolute damage triggering a scale tick
    float scale_increment = 0.0f;
    // Percent chance (at damage threshold)
    int chance = 100;
    // Chance scaling for damage above the threshold
    float chance_dmg_scaling = 0.0f;
    // Intensity applied at the damage threshold.
    int intensity = 1;
    // Intensity scaling for damage above the threshold.
    float intensity_dmg_scaling = 0.0f;
    // Duration in turns at the damage threshold.
    int duration = 1;
    // Duration scaling for damage above the threshold.
    float duration_dmg_scaling = 0.0f;
    // Max intensity applied via damage (direct effect addition is exempt)
    int max_intensity = INT_MAX;
    // Max duration applied via damage (direct effect addition is exempt)
    int max_duration = INT_MAX;

    void load( const JsonObject &jo );
};

struct body_part_type {
    public:
        /**
         * the different types of body parts there are.
         * this allows for the ability to group limbs or determine a limb of a certain type
         */
        enum class type {
            // this is where helmets go, and is a vital part.
            head,
            // the torso is generally the center of mass of a creature
            torso,
            // provides sight
            sensor,
            // you eat and scream with this
            mouth,
            // may manipulate objects to some degree, is a main part
            arm,
            // manipulates objects. usually is not a main part.
            hand,
            // provides motive power
            leg,
            // helps with balance. usually is not a main part
            foot,
            // may reduce fall damage
            wing,
            // may provide balance or manipulation
            tail,
            // more of a general purpose limb, such as horns.
            other,
            num_types
        };

        std::vector<std::pair<bodypart_str_id, mod_id>> src;

        /** Sub-location of the body part used for encumberance, coverage and determining protection
         */
        std::vector<sub_bodypart_str_id> sub_parts;

        std::map<units::mass, int> encumbrance_per_weight;

        cata::flat_set<json_character_flag> flags;
        cata::flat_set<json_character_flag> conditional_flags;

    private:
        // limb score values
        std::map<limb_score_id, bp_limb_score> limb_scores;
        damage_instance damage;

    public:
        bodypart_str_id id;
        // Legacy "string id"
        std::string legacy_id;
        // "Parent" of this part for damage purposes - main parts are their own "parents"
        bodypart_str_id main_part;
        // "Parent" of this part for connectedness - should be next part towards head.
        // Head connects to itself.
        bodypart_str_id connected_to;
        // A part that has no opposite is its own opposite (that's pretty Zen)
        bodypart_str_id opposite_part;

        // A weighted list of limb types. The type with the highest weight is the primary type
        std::map<body_part_type::type, float> limbtypes;

        // Limb-specific attacks
        std::set<matec_id> techniques;

        // Effects to trigger on getting hit
        std::vector<bp_onhit_effect> effects_on_hit;

        // Those are stored untranslated
        translation name;
        translation name_multiple;
        translation accusative;
        translation accusative_multiple;
        translation name_as_heading;
        translation name_as_heading_multiple;
        translation smash_message;
        translation hp_bar_ui_text;
        translation encumb_text;
        // Legacy enum "int id"
        body_part token = num_bp;
        /** Size of the body part for melee targeting. */
        float hit_size = 0.0f;
        /**
         * How hard is it to hit a given body part, assuming "owner" is hit.
         * Higher number means good hits will veer towards this part,
         * lower means this part is unlikely to be hit by inaccurate attacks.
         * Formula is `chance *= pow(hit_roll, hit_difficulty)`
         */
        float hit_difficulty = 0.0f;

        // Parts with no opposites have BOTH here
        side part_side = side::BOTH;

        // Threshold to start encumbrance scaling
        int encumbrance_threshold = 0;
        // Limit of encumbrance, after reaching this point the limb contributes no scores
        int encumbrance_limit = 0;
        // Health at which the limb stops contributing its conditional flags / techs
        int health_limit = 0;

        // Minimum BMI to start adding extra encumbrance (only counts the points of BMI that came from fat, ignoring muscle and bone)
        int bmi_encumbrance_threshold = 999;
        // Amount of encumbrance per point of BMI over the threshold
        float bmi_encumbrance_scalar = 0;
        float smash_efficiency = 0.5f;

        //Morale parameters
        float hot_morale_mod = 0.0f;
        float cold_morale_mod = 0.0f;
        float stylish_bonus = 0.0f;
        int squeamish_penalty = 0;
        bool feels_discomfort = true;

        units::temperature_delta fire_warmth_bonus = 0_C_delta;

        //Innate environmental protection
        int env_protection = 0;

        int base_hp = 60;
        // Innate healing rate of the bodypart
        int heal_bonus = 0;
        float mend_rate = 1.0f;

        // Ugliness of bodypart, can be mitigated by covering them up
        int ugliness = 0;
        // Ugliness that can't be covered (obviously nonstandard anatomy, even under bulky armor)
        int ugliness_mandatory = 0;

        // Intrinsic temperature bonus of the bodypart
        units::temperature_delta temp_min = 0_C_delta;
        // Temperature bonus to apply when not overheated
        units::temperature_delta temp_max = 0_C_delta;
        int drench_max = 0;
        int drench_increment = 2;
        int drying_chance = 1;
        int drying_increment = 1;
        // Wetness morale bonus/malus of the limb
        int wet_morale = 0;
        int technique_enc_limit = 50;

    private:
        int bionic_slots_ = 0;
        body_part_type::type _primary_limb_type = body_part_type::type::num_types;
        // Protection from various damage types
        resistances armor;

    public:
        stat_hp_mods hp_mods;
        bool unarmed_bonus = false;
        // if a limb is vital and at 0 hp, you die.
        bool is_vital = false;
        bool is_limb = false;

        bool was_loaded = false;

        bool has_flag( const json_character_flag &flag ) const;
        body_part_type::type primary_limb_type() const;
        bool has_type( const body_part_type::type &type ) const;

        // return a random sub part from the weighted list of subparts
        // if secondary is true instead returns a part from only the secondary sublocations
        sub_bodypart_id random_sub_part( bool secondary ) const;

        void load( const JsonObject &jo, std::string_view src );
        void finalize();
        void check() const;

        static void load_bp( const JsonObject &jo, const std::string &src );
        static const std::vector<body_part_type> &get_all();

        // Clears all bps
        static void reset();
        // Post-load finalization
        static void finalize_all();
        // Verifies that body parts make sense
        static void check_consistency();

        float get_limb_score( const limb_score_id &id ) const;
        float get_limb_score_max( const limb_score_id &id ) const;
        bool has_limb_score( const limb_score_id &id ) const;

        int bionic_slots() const {
            return bionic_slots_;
        }

        float unarmed_damage( const damage_type_id &dt ) const;
        float unarmed_arpen( const damage_type_id &dt ) const;

        float damage_resistance( const damage_type_id &dt ) const;
        float damage_resistance( const damage_unit &du ) const;

        // combine matching body part and subbodypart strings together for printing
        static std::set<translation, localized_comparator> consolidate( std::vector<sub_bodypart_id>
                &covered );

        // this version just pairs normal body parts
        static std::set<translation, localized_comparator> consolidate( std::vector<bodypart_id>
                &covered );
};

template<>
struct enum_traits<body_part_type::type> {
    static constexpr body_part_type::type last = body_part_type::type::num_types;
};

struct layer_details {

    std::vector<int> pieces;
    int max = 0;
    int total = 0;

    // if the layer is conflicting
    bool is_conflicting = false;

    std::vector<sub_bodypart_id> covered_sub_parts;

    void reset();
    int layer( int encumbrance, bool conflicts );

    bool operator ==( const layer_details &rhs ) const {
        return max == rhs.max &&
               total == rhs.total &&
               pieces == rhs.pieces;
    }
};

struct encumbrance_data {
    int encumbrance = 0;
    int armor_encumbrance = 0;
    int layer_penalty = 0;

    std::array<layer_details, static_cast<size_t>( layer_level::NUM_LAYER_LEVELS )>
    layer_penalty_details;

    bool add_sub_location( layer_level level, sub_bodypart_id sbp );

    bool add_sub_location( layer_level level, sub_bodypart_str_id sbp );

    void layer( const layer_level level, const int encumbrance, bool conflicts ) {
        layer_penalty_details[static_cast<size_t>( level )].layer( encumbrance, conflicts );
    }

    void reset() {
        *this = encumbrance_data();
    }

    bool operator ==( const encumbrance_data &rhs ) const {
        return encumbrance == rhs.encumbrance &&
               armor_encumbrance == rhs.armor_encumbrance &&
               layer_penalty == rhs.layer_penalty &&
               layer_penalty_details == rhs.layer_penalty_details;
    }
};

class bodypart
{
    private:
        bodypart_str_id id;

        int hp_cur = 0;
        int hp_max = 0;

        int wetness = 0;
        units::temperature temp_cur = BODYTEMP_NORM;
        units::temperature temp_conv = BODYTEMP_NORM;
        int frostbite_timer = 0;

        int healed_total = 0;
        int damage_bandaged = 0;
        int damage_disinfected = 0;

        encumbrance_data encumb_data; // NOLINT(cata-serialize)

        std::array<int, NUM_WATER_TOLERANCE> mut_drench; // NOLINT(cata-serialize)

        // adjust any limb "value" based on how wounded the limb is. scaled to 0-75%
        float wound_adjusted_limb_value( float val ) const;
        // Same idea as for wounds, though not all scores get this applied. Should be applied after wounds.
        float encumb_adjusted_limb_value( float val ) const;
        // If the limb score is affected by a skill, adjust it by the skill's level (used for swimming)
        float skill_adjusted_limb_value( float val, int skill ) const;
    public:
        bodypart(): id( bodypart_str_id::NULL_ID() ), mut_drench() {}
        explicit bodypart( bodypart_str_id id ): id( id ), hp_cur( id->base_hp ), hp_max( id->base_hp ),
            mut_drench() {}

        bodypart_id get_id() const;

        void set_hp_to_max();
        bool is_at_max_hp() const;

        float get_wetness_percentage() const;

        int get_encumbrance_threshold() const;
        // Check if we're above our encumbrance limit
        bool is_limb_overencumbered() const;
        bool has_conditional_flag( const json_character_flag &flag ) const;

        // Get our limb attacks
        std::set<matec_id> get_limb_techs() const;

        // Get onhit effects
        std::vector<bp_onhit_effect> get_onhit_effects( damage_type_id dtype ) const;

        // Get modified limb score as defined in limb_scores.json.
        // override forces the limb score to be affected by encumbrance/wounds (-1 == no override).
        float get_limb_score( const limb_score_id &score, int skill = -1, int override_encumb = -1,
                              int override_wounds = -1 ) const;
        float get_limb_score_max( const limb_score_id &score ) const;

        int get_hp_cur() const;
        int get_hp_max() const;
        int get_healed_total() const;
        int get_damage_bandaged() const;
        int get_damage_disinfected() const;
        int get_drench_capacity() const;
        int get_wetness() const;
        int get_frostbite_timer() const;
        units::temperature get_temp_cur() const;
        units::temperature get_temp_conv() const;
        int get_bmi_encumbrance_threshold() const;
        float get_bmi_encumbrance_scalar() const;

        std::array<int, NUM_WATER_TOLERANCE> get_mut_drench() const;

        const encumbrance_data &get_encumbrance_data() const;

        void set_hp_cur( int set );
        void set_hp_max( int set );
        void set_healed_total( int set );
        void set_damage_bandaged( int set );
        void set_damage_disinfected( int set );
        void set_wetness( int set );
        void set_temp_cur( units::temperature set );
        void set_temp_conv( units::temperature set );
        void set_frostbite_timer( int set );

        void set_encumbrance_data( const encumbrance_data &set );

        void set_mut_drench( const std::pair<water_tolerance, int> &set );

        void mod_hp_cur( int mod );
        void mod_hp_max( int mod );
        void mod_healed_total( int mod );
        void mod_damage_bandaged( int mod );
        void mod_damage_disinfected( int mod );
        void mod_wetness( int mod );
        void mod_temp_cur( units::temperature_delta mod );
        void mod_temp_conv( units::temperature_delta mod );
        void mod_frostbite_timer( int mod );

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &jo );
};

/** Returns the new id for old token */
const bodypart_str_id &convert_bp( body_part bp );

/** Returns the opposite side. */
side opposite_side( side s );

// identify the index of a body part's "other half", or itself if not
const std::array<size_t, 12> bp_aiOther = {{0, 1, 2, 3, 5, 4, 7, 6, 9, 8, 11, 10}};

/** Returns the matching name of the body_part token. */
std::string body_part_name( const bodypart_id &bp, int number = 1 );

/** Returns the matching accusative name of the body_part token, i.e. "Shrapnel hits your X".
 *  These are identical to body_part_name above in English, but not in some other languages. */
std::string body_part_name_accusative( const bodypart_id &bp, int number = 1 );

/** Returns the name of the body parts in a context where the name is used as
 * a heading or title e.g. "Left Arm". */
std::string body_part_name_as_heading( const bodypart_id &bp, int number );

/** Returns the body part text to be displayed in the HP bar */
std::string body_part_hp_bar_ui_text( const bodypart_id &bp );

/** Returns the matching encumbrance text for a given body_part token. */
std::string encumb_text( const bodypart_id &bp );

#endif // CATA_SRC_BODYPART_H
