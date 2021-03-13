#pragma once
#ifndef CATA_SRC_MAGIC_ENCHANTMENT_H
#define CATA_SRC_MAGIC_ENCHANTMENT_H

#include <iosfwd>
#include <map>
#include <new>
#include <set>
#include <utility>
#include <vector>

#include "calendar.h"
#include "magic.h"
#include "optional.h"
#include "type_id.h"
#include "units_fwd.h"

class Character;
class Creature;
class JsonObject;
class JsonOut;
class item;

namespace enchant_vals
{
// the different types of values that can be modified by enchantments
// either the item directly or the Character, whichever is more appropriate
enum class mod : int {
    // effects for the Character
    STRENGTH,
    DEXTERITY,
    PERCEPTION,
    INTELLIGENCE,
    SPEED,
    ATTACK_COST,
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
    THIRST,        // thirst rate
    FATIGUE,       // fatigue rate
    PAIN,          // cost or regen over time
    BONUS_DODGE,
    BONUS_BLOCK,
    BONUS_DAMAGE,
    ATTACK_NOISE,
    SHOUT_NOISE,
    FOOTSTEP_NOISE,
    SIGHT_RANGE,
    CARRY_WEIGHT,
    SOCIAL_LIE,
    SOCIAL_PERSUADE,
    SOCIAL_INTIMIDATE,
    SLEEPY,
    ARMOR_BASH,
    ARMOR_CUT,
    ARMOR_STAB,
    ARMOR_BULLET,
    ARMOR_HEAT,
    ARMOR_COLD,
    ARMOR_ELEC,
    ARMOR_ACID,
    ARMOR_BIO,
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
    ITEM_DAMAGE_AP,      // armor piercing
    ITEM_ARMOR_BASH,
    ITEM_ARMOR_CUT,
    ITEM_ARMOR_STAB,
    ITEM_ARMOR_BULLET,
    ITEM_ARMOR_HEAT,
    ITEM_ARMOR_COLD,
    ITEM_ARMOR_ELEC,
    ITEM_ARMOR_ACID,
    ITEM_ARMOR_BIO,
    ITEM_WEIGHT,
    ITEM_ENCUMBRANCE,
    ITEM_VOLUME,
    ITEM_COVERAGE,
    ITEM_ATTACK_SPEED,
    ITEM_WET_PROTECTION,
    NUM_MOD
};
} // namespace enchant_vals

// an "enchantment" is what passive artifact effects used to be:
// under certain conditions, the effect persists upon the appropriate Character
class enchantment
{
    public:
        // if a Character "has" an enchantment, it is viable to check for the condition
        enum has {
            WIELD,
            WORN,
            HELD,
            NUM_HAS
        };
        // the condition at which the enchantment is giving passive effects
        enum condition {
            ALWAYS,
            UNDERGROUND,
            UNDERWATER,
            ACTIVE, // the item, mutation, etc. is active
            NUM_CONDITION
        };

        static void load_enchantment( const JsonObject &jo, const std::string &src );
        void load( const JsonObject &jo, const std::string &src = "" );

        // attempts to add two like enchantments together.
        // if their conditions don't match, return false. else true.
        bool add( const enchantment &rhs );

        // adds two enchantments together and ignores their conditions
        void force_add( const enchantment &rhs );

        void set_has( has value );

        void add_value_add( enchant_vals::mod value, int add_value );
        void add_value_mult( enchant_vals::mod value, float mult_value );

        void add_hit_me( const fake_spell &sp );
        void add_hit_you( const fake_spell &sp );

        int get_value_add( enchant_vals::mod value ) const;
        double get_value_multiply( enchant_vals::mod value ) const;
        // the standard way of modifying a value, adds then multiplies.
        double modify_value( enchant_vals::mod mod_val, double value ) const;
        units::energy modify_value( enchant_vals::mod mod_val, units::energy value ) const;
        units::mass modify_value( enchant_vals::mod mod_val, units::mass value ) const;

        // this enchantment has a valid condition and is in the right location
        bool is_active( const Character &guy, const item &parent ) const;

        // this enchantment has a valid item independent conditions
        // @active means the container for the enchantment is active, for comparison to active flag.
        bool is_active( const Character &guy, bool active ) const;

        // this enchantment is active when wielded.
        // shows total conditional values, so only use this when Character is not available
        bool active_wield() const;

        // modifies character stats, or does other passive effects
        void activate_passive( Character &guy ) const;

        enchantment_id id;

        bool was_loaded = false;

        void serialize( JsonOut &jsout ) const;

        // casts all the hit_you_effects on the target
        void cast_hit_you( Character &caster, const Creature &target ) const;
        // casts all the hit_me_effects on self or a target depending on the enchantment definition
        void cast_hit_me( Character &caster, const Creature *target ) const;

        const std::set<trait_id> &get_mutations() const {
            return mutations;
        }

        bool operator==( const enchantment &rhs ) const;
    private:
        std::set<trait_id> mutations;
        cata::optional<emit_id> emitter;
        std::map<efftype_id, int> ench_effects;
        // values that add to the base value
        std::map<enchant_vals::mod, int> values_add;
        // values that get multiplied to the base value
        // multipliers add to each other instead of multiply against themselves
        std::map<enchant_vals::mod, double> values_multiply;

        std::vector<fake_spell> hit_me_effect;
        std::vector<fake_spell> hit_you_effect;

        std::map<time_duration, std::vector<fake_spell>> intermittent_activation;

        std::pair<has, condition> active_conditions;

        void add_activation( const time_duration &dur, const fake_spell &fake );

        // checks if the enchantments have the same active_conditions
        bool stacks_with( const enchantment &rhs ) const;

        int mult_bonus( enchant_vals::mod value_type, int base_value ) const;

        // performs cooldown and distance checks before casting enchantment spells
        void cast_enchantment_spell( Character &caster, const Creature *target,
                                     const fake_spell &sp ) const;
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
