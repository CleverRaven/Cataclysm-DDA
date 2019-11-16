#pragma once
#ifndef MAGIC_ENCHANTMENT_H
#define MAGIC_ENCHANTMENT_H

#include <map>
#include <string>
#include <vector>

#include "magic.h"
#include "type_id.h"

class Character;
class item;
class JsonOut;
class time_duration;

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
            NUM_CONDITION
        };
        // the different types of values that can be modified by enchantments
        // either the item directly or the Character, whichever is more appropriate
        enum mod {
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
            THIRST,        // cost or regen over time
            FATIGUE,       // cost or regen over time
            PAIN,          // cost or regen over time
            BONUS_DODGE,
            BONUS_BLOCK,
            BONUS_DAMAGE,
            ATTACK_NOISE,
            SPELL_NOISE,
            SHOUT_NOISE,
            FOOTSTEP_NOISE,
            SIGHT_RANGE,
            CARRY_WEIGHT,
            CARRY_VOLUME,
            SOCIAL_LIE,
            SOCIAL_PERSUADE,
            SOCIAL_INTIMIDATE,
            ARMOR_BASH,
            ARMOR_CUT,
            ARMOR_STAB,
            ARMOR_HEAT,
            ARMOR_COLD,
            ARMOR_ELEC,
            ARMOR_ACID,
            ARMOR_BIO,
            // effects for the item that has the enchantment
            ITEM_DAMAGE_BASH,
            ITEM_DAMAGE_CUT,
            ITEM_DAMAGE_STAB,
            ITEM_DAMAGE_HEAT,
            ITEM_DAMAGE_COLD,
            ITEM_DAMAGE_ELEC,
            ITEM_DAMAGE_ACID,
            ITEM_DAMAGE_BIO,
            ITEM_DAMAGE_AP,      // armor piercing
            ITEM_ARMOR_BASH,
            ITEM_ARMOR_CUT,
            ITEM_ARMOR_STAB,
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

        static void load_enchantment( JsonObject &jo, const std::string &src );
        void load( JsonObject &jo, const std::string &src = "" );

        // attempts to add two like enchantments together.
        // if their conditions don't match, return false. else true.
        bool add( const enchantment &rhs );

        // adds two enchantments together and ignores their conditions
        void force_add( const enchantment &rhs );

        int get_value_add( mod value ) const;
        double get_value_multiply( mod value ) const;

        // this enchantment has a valid condition and is in the right location
        bool is_active( const Character &guy, const item &parent ) const;

        // this enchantment is active when wielded.
        // shows total conditional values, so only use this when Character is not available
        bool active_wield() const;

        // modifies character stats, or does other passive effects
        void activate_passive( Character &guy ) const;

        enchantment_id id;

        bool was_loaded;

        void serialize( JsonOut &jsout ) const;

        // casts all the hit_you_effects on the target
        void cast_hit_you( Character &caster, const tripoint &target ) const;
        // casts all the hit_me_effects on self
        void cast_hit_me( Character &caster ) const;
    private:
        // values that add to the base value
        std::map<mod, int> values_add;
        // values that get multiplied to the base value
        // multipliers add to each other instead of multiply against themselves
        std::map<mod, double> values_multiply;

        std::vector<fake_spell> hit_me_effect;
        std::vector<fake_spell> hit_you_effect;

        std::map<time_duration, std::vector<fake_spell>> intermittent_activation;

        std::pair<has, condition> active_conditions;

        void add_activation( const time_duration &dur, const fake_spell &fake );

        // checks if the enchantments have the same active_conditions
        bool stacks_with( const enchantment &rhs ) const;

        int mult_bonus( mod value_type, int base_value ) const;
};

#endif
