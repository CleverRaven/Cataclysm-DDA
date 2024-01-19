#pragma once
#ifndef CATA_SRC_MAGIC_ENCHANTMENT_H
#define CATA_SRC_MAGIC_ENCHANTMENT_H

#include <iosfwd>
#include <map>
#include <new>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include "calendar.h"
#include "dialogue_helpers.h"
#include "magic.h"
#include "type_id.h"
#include "units_fwd.h"

class Character;
class Creature;
class JsonObject;
class JsonOut;
class item;
struct dialogue;
struct dbl_or_var;
namespace enchant_vals
{
// the different types of values that can be modified by enchantments
// either the item directly or the Character, whichever is more appropriate
enum class mod : int {
    //tracker for artifact carrying penalties
    ARTIFACT_RESONANCE,
    // effects for the Character
    STRENGTH,
    DEXTERITY,
    PERCEPTION,
    INTELLIGENCE,
    SPEED,
    ATTACK_SPEED, // affects attack speed of item even if it's not the one you're wielding
    MOVE_COST,
    METABOLISM,
    MAX_MANA,
    REGEN_MANA,
    BIONIC_POWER,
    MAX_STAMINA,
    REGEN_STAMINA,
    MAX_HP,        // for all limbs! use with caution
    REGEN_HP,
    HUNGER,        // hunger rate
    THIRST,        // thirst rate
    FATIGUE,       // fatigue rate
    PAIN,
    PAIN_REMOVE,
    BONUS_DODGE,
    BONUS_BLOCK,
    MELEE_DAMAGE,
    RANGED_DAMAGE,
    ATTACK_NOISE,
    SHOUT_NOISE,
    FOOTSTEP_NOISE,
    SIGHT_RANGE_ELECTRIC,
    MOTION_VISION_RANGE,
    SIGHT_RANGE_FAE,
    SIGHT_RANGE_NETHER,
    SIGHT_RANGE_MINDS,
    CARRY_WEIGHT,
    WEAPON_DISPERSION,
    SOCIAL_LIE,
    SOCIAL_PERSUADE,
    SOCIAL_INTIMIDATE,
    SLEEPY,
    LUMINATION,
    EFFECTIVE_HEALTH_MOD,
    MOD_HEALTH,
    MOD_HEALTH_CAP,
    READING_EXP,
    SKILL_RUST_RESIST,
    LEARNING_FOCUS,
    ARMOR_BASH,
    ARMOR_CUT,
    ARMOR_STAB,
    ARMOR_BULLET,
    ARMOR_HEAT,
    ARMOR_COLD,
    ARMOR_ELEC,
    ARMOR_ACID,
    ARMOR_BIO,
    EXTRA_BASH,
    EXTRA_CUT,
    EXTRA_STAB,
    EXTRA_BULLET,
    EXTRA_HEAT,
    EXTRA_COLD,
    EXTRA_ELEC,
    EXTRA_ACID,
    EXTRA_BIO,
    EXTRA_ELEC_PAIN,
    RECOIL_MODIFIER, //affects recoil when shooting a gun
    // effects for the item that has the enchantment
    ITEM_DAMAGE_PURE,
    ITEM_DAMAGE_BASH,
    ITEM_DAMAGE_CUT,
    ITEM_DAMAGE_STAB,
    ITEM_DAMAGE_BULLET,
    ITEM_DAMAGE_HEAT,
    ITEM_DAMAGE_COLD,
    ITEM_DAMAGE_ELEC,
    ITEM_DAMAGE_ACID,
    ITEM_DAMAGE_BIO,
    ITEM_ARMOR_BASH,
    ITEM_ARMOR_CUT,
    ITEM_ARMOR_STAB,
    ITEM_ARMOR_BULLET,
    ITEM_ARMOR_HEAT,
    ITEM_ARMOR_COLD,
    ITEM_ARMOR_ELEC,
    ITEM_ARMOR_ACID,
    ITEM_ARMOR_BIO,
    ITEM_ATTACK_SPEED,
    CLIMATE_CONTROL_HEAT,
    CLIMATE_CONTROL_CHILL,
    COMBAT_CATCHUP,
    KNOCKBACK_RESIST,
    KNOCKDOWN_RESIST,
    FALL_DAMAGE,
    FORCEFIELD,
    EVASION,
    OVERKILL_DAMAGE,
    RANGE,
    AVOID_FRIENDRY_FIRE,
    NUM_MOD
};
} // namespace enchant_vals

// an "enchantment" is what passive artifact effects used to be:
// under certain conditions, the effect persists upon the appropriate Character
class enchantment
{
    public:
        enum has {
            WIELD,
            WORN,
            HELD,
            NUM_HAS
        };
        // the condition at which the enchantment is giving passive effects
        enum condition {
            ALWAYS,
            ACTIVE, // the item, mutation, etc. is active
            INACTIVE, // the item, mutation, etc. is inactive
            DIALOG_CONDITION, // Check a provided dialog condition
            NUM_CONDITION
        };

        static void load_enchantment( const JsonObject &jo, const std::string &src );
        static void reset();
        void load( const JsonObject &jo, std::string_view src = {},
                   const std::optional<std::string> &inline_id = std::nullopt, bool is_child = false );

        // Takes in a JsonValue which can be either a string or an enchantment object and returns the id of the enchantment the caller will use.
        // If the input is a string return it as an enchantment_id otherwise create an enchantment with id inline_id and return inline_id as an enchantment id
        static enchantment_id load_inline_enchantment( const JsonValue &jv, std::string_view src,
                std::string &inline_id );

        // this enchantment has a valid condition and is in the right location
        bool is_active( const Character &guy, const item &parent ) const;

        // this enchantment has a valid item independent conditions
        // @active means the container for the enchantment is active, for comparison to active flag.
        bool is_active( const Character &guy, bool active ) const;

        // this enchantment is active when wielded.
        // shows total conditional values, so only use this when Character is not available
        bool active_wield() const;
        enchantment_id id;
        // NOLINTNEXTLINE(cata-serialize)
        std::vector<std::pair<enchantment_id, mod_id>> src;

        bool was_loaded = false;

        const std::set<trait_id> &get_mutations() const {
            return mutations;
        }
        double get_value_add( enchant_vals::mod value, const Character &guy ) const;
        double get_value_multiply( enchant_vals::mod value, const Character &guy ) const;

        body_part_set modify_bodyparts( const body_part_set &unmodified ) const;
        // does the enchantment modify bodyparts?
        bool modifies_bodyparts() const;
        struct bodypart_changes {
            bodypart_str_id gain;
            bodypart_str_id lose;
            bool was_loaded = false;
            void serialize( JsonOut &jsout ) const;
            void deserialize( const JsonObject &jo );
            void load( const JsonObject &jo );
        };
        std::vector<bodypart_changes> modified_bodyparts;

        std::set<trait_id> mutations;
        std::optional<emit_id> emitter;
        std::map<efftype_id, int> ench_effects;

        // name to display for enchantments on items
        translation name;

        // description to display for enchantments on items
        translation description;

        // values that add to the base value
        std::map<enchant_vals::mod, dbl_or_var> values_add; // NOLINT(cata-serialize)
        // values that get multiplied to the base value
        // multipliers add to each other instead of multiply against themselves
        std::map<enchant_vals::mod, dbl_or_var> values_multiply; // NOLINT(cata-serialize)

        // the exact same as above, though specifically for skills
        std::map<skill_id, dbl_or_var> skill_values_add; // NOLINT(cata-serialize)
        std::map<skill_id, dbl_or_var> skill_values_multiply; // NOLINT(cata-serialize)

        std::vector<fake_spell> hit_me_effect;
        std::vector<fake_spell> hit_you_effect;

        std::map<time_duration, std::vector<fake_spell>> intermittent_activation;

        std::pair<has, condition> active_conditions;
        std::function<bool( dialogue & )> dialog_condition; // NOLINT(cata-serialize)

        void add_activation( const time_duration &dur, const fake_spell &fake );
};

class enchant_cache : public enchantment
{
    public:

        double modify_value( enchant_vals::mod mod_val, double value ) const;
        double modify_value( const skill_id &mod_val, double value ) const;
        units::energy modify_value( enchant_vals::mod mod_val, units::energy value ) const;
        units::mass modify_value( enchant_vals::mod mod_val, units::mass value ) const;
        // adds two enchantments together and ignores their conditions
        void force_add( const enchantment &rhs, const Character &guy );
        void force_add( const enchant_cache &rhs );

        // modifies character stats, or does other passive effects
        void activate_passive( Character &guy ) const;
        double get_value_add( enchant_vals::mod value ) const;
        double get_value_multiply( enchant_vals::mod value ) const;
        int mult_bonus( enchant_vals::mod value_type, int base_value ) const;

        int get_skill_value_add( const skill_id &value ) const;
        double get_skill_value_multiply( const skill_id &value ) const;
        int skill_mult_bonus( const skill_id &value_type, int base_value ) const;
        // attempts to add two like enchantments together.
        // if their conditions don't match, return false. else true.
        bool add( const enchantment &rhs, Character &you );
        bool add( const enchant_cache &rhs );
        // checks if the enchantments have the same active_conditions
        bool stacks_with( const enchantment &rhs ) const;
        // performs cooldown and distance checks before casting enchantment spells
        void cast_enchantment_spell( Character &caster, const Creature *target,
                                     const fake_spell &sp ) const;

        // casts all the hit_you_effects on the target
        void cast_hit_you( Character &caster, const Creature &target ) const;
        // casts all the hit_me_effects on self or a target depending on the enchantment definition
        void cast_hit_me( Character &caster, const Creature *target ) const;
        void serialize( JsonOut &jsout ) const;
        void add_value_add( enchant_vals::mod value, int add_value );

        void set_has( enchantment::has value );
        void add_value_mult( enchant_vals::mod value, float mult_value );
        void add_hit_you( const fake_spell &sp );
        void add_hit_me( const fake_spell &sp );
        void load( const JsonObject &jo, std::string_view src = {},
                   const std::optional<std::string> &inline_id = std::nullopt );
        bool operator==( const enchant_cache &rhs ) const;

        // details of each enchantment that includes them (name and description)
        std::vector<std::pair<std::string, std::string>> details; // NOLINT(cata-serialize)

    private:
        std::map<enchant_vals::mod, double> values_add; // NOLINT(cata-serialize)
        // values that get multiplied to the base value
        // multipliers add to each other instead of multiply against themselves
        std::map<enchant_vals::mod, double> values_multiply; // NOLINT(cata-serialize)

        // the exact same as above, though specifically for skills
        std::map<skill_id, int> skill_values_add; // NOLINT(cata-serialize)
        std::map<skill_id, int> skill_values_multiply; // NOLINT(cata-serialize)
};

template <typename E> struct enum_traits;

template<>
struct enum_traits<enchantment::has> {
    static constexpr enchantment::has last = enchantment::has::NUM_HAS;
};

template<>
struct enum_traits<enchantment::condition> {
    static constexpr enchantment::condition last = enchantment::condition::NUM_CONDITION;
};

template<>
struct enum_traits<enchant_vals::mod> {
    static constexpr enchant_vals::mod last = enchant_vals::mod::NUM_MOD;
};

#endif // CATA_SRC_MAGIC_ENCHANTMENT_H
