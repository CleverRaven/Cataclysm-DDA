#pragma once
#ifndef CATA_SRC_WEAKPOINT_H
#define CATA_SRC_WEAKPOINT_H

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "translation.h"
#include "type_id.h"

class Character;
class Creature;
class JsonArray;
class JsonObject;
class JsonValue;
class item;
class time_duration;
struct const_dialogue;
struct damage_instance;
struct resistances;

// Information about an attack on a weak point.
struct weakpoint_attack {
    enum class attack_type : int {
        NONE, // Unusual damage instances, such as falls, spells, and effects.
        MELEE_BASH, // Melee bludgeoning attacks
        MELEE_CUT, // Melee slashing attacks
        MELEE_STAB, // Melee piercing attacks
        PROJECTILE, // Ranged projectile attacks, including throwing weapons and guns

        NUM,
    };

    // The source of the attack.
    Creature *source;
    // The target of the attack.
    Creature *target;
    // The weapon used to make the attack.
    const item *weapon;
    // The type of the attack.
    attack_type type;
    // Whether the attack from a thrown object.
    bool is_thrown;
    /**
    * How accurate the ranged attack is.
    * -1.0 means not a ranged attack.
    */
    double accuracy = -1.0;
    // Whether the attack a critical hit.
    bool is_crit;
    // The Creature's skill in hitting weak points.
    float wp_skill;

    weakpoint_attack();
    // Returns the attack type of a melee attack.
    static attack_type type_of_melee_attack( const damage_instance &damage );
    // Compute and set the value of `wp_skill`.
    void compute_wp_skill();
};

// An effect that a weakpoint can cause.
struct weakpoint_effect {
    // The type of the effect.
    efftype_id effect;
    // Effect on condition, that would be run.
    std::vector<effect_on_condition_id> effect_on_conditions;
    // The percent chance of causing the effect.
    float chance;
    // Whether the effect is permanent.
    bool permanent;
    // The range of the durations (in turns) of the effect.
    std::pair<int, int> duration;
    // The range of the intensities of the effect.
    std::pair<int, int> intensity;
    // The range of damage, as a percentage of max health, required to the effect.
    std::pair<float, float> damage_required;
    // The message to print, if the player causes the effect.
    translation message;

    weakpoint_effect();
    // Gets translated effect message
    std::string get_message() const;
    // Maybe apply an effect to the target.
    void apply_to( Creature &target, int total_damage, const weakpoint_attack &attack ) const;
    void load( const JsonObject &jo );
};

struct weakpoint_difficulty {
    std::array<float, static_cast<int>( weakpoint_attack::attack_type::NUM )> difficulty;

    explicit weakpoint_difficulty( float default_value );
    float of( const weakpoint_attack &attack ) const;
    void load( const JsonObject &jo );
};

struct weakpoint_family {
    // ID of the family. Equal to the proficiency, if not provided.
    std::string id;
    // Name of proficiency corresponding to the family.
    proficiency_id proficiency;
    // The skill bonus for having the proficiency.
    std::optional<float> bonus;
    // The skill penalty for not having the proficiency.
    std::optional<float> penalty;

    float modifier( const Character &attacker ) const;
    void load( const JsonValue &jsin );
};

struct weakpoint_families {
    // List of weakpoint families
    std::vector<weakpoint_family> families;

    // Practice all weak point families for the given duration. Returns true if a proficiency was practiced.
    bool practice( Character &learner, const time_duration &amount ) const;
    bool practice_hit( Character &learner ) const;
    bool practice_kill( Character &learner ) const;
    bool practice_dissect( Character &learner ) const;
    float modifier( const Character &attacker ) const;

    void clear();
    void load( const JsonArray &ja );
    void remove( const JsonArray &ja );
};

struct weakpoint {
    // ID of the weakpoint. Equal to the name, if not provided.
    std::string id;
    // Name of the weakpoint. Can be empty.
    translation name;
    // Percent chance of hitting the weakpoint. Can be increased by skill.
    float coverage = 100.0f;
    // Separate wp that benefit attacker and hurt monster from wp that do not
    bool is_good = true;
    // Multiplier for existing armor values. Defaults to 1.
    std::unordered_map<damage_type_id, float> armor_mult;
    // Flat penalty to armor values. Applied after the multiplier.
    std::unordered_map<damage_type_id, float> armor_penalty;
    // Damage multipliers. Applied after armor.
    std::unordered_map<damage_type_id, float> damage_mult;
    // Critical damage multipliers. Applied after armor instead of damage_mult, if the attack is a crit.
    std::unordered_map<damage_type_id, float> crit_mult;
    // Dialogue conditions of weakpoint
    std::function<bool( const_dialogue const & )> condition;
    bool has_condition = false;
    // A list of effects that may trigger by hitting this weak point.
    std::vector<weakpoint_effect> effects;
    // Constant coverage multipliers, depending on the attack type.
    weakpoint_difficulty coverage_mult;
    // Difficulty gates, varying by the attack type.
    weakpoint_difficulty difficulty;
    bool is_head = false;

    weakpoint();
    // Gets translated name
    std::string get_name() const;
    // Apply the armor multipliers and offsets to a set of resistances.
    void apply_to( resistances &resistances ) const;
    // Apply the damage multipliers to a set of damage values.
    void apply_to( damage_instance &damage, bool is_crit ) const;
    void apply_effects( Creature &target, int total_damage, const weakpoint_attack &attack ) const;
    // Return the change of the creature hitting the weakpoint.
    float hit_chance( const weakpoint_attack &attack ) const;
    void load( const JsonObject &jo );
    void check() const;
};

struct weakpoints {
    // id of this set of weakpoints (inline weakpoints have a null id)
    weakpoints_id id;
    std::vector<std::pair<weakpoints_id, mod_id>> src;
    // List of weakpoints. Each weakpoint should have a unique id.
    std::vector<weakpoint> weakpoint_list;
    // Default weakpoint to return.
    weakpoint default_weakpoint;
    // TODO: make private
    bool was_loaded = false;

    // Selects a weakpoint to hit.
    const weakpoint *select_weakpoint( const weakpoint_attack &attack ) const;

    void clear();
    // load inline definition
    void load( const JsonArray &ja );
    void remove( const JsonArray &ja );
    void finalize();
    void check() const;

    /********************* weakpoint_set handling ****************************/
    // load standalone JSON type
    void load( const JsonObject &jo, std::string_view src );
    void add_from_set( const weakpoints_id &set_id, bool replace_id );
    void add_from_set( const weakpoints &set, bool replace_id );
    void del_from_set( const weakpoints_id &set_id );
    void del_from_set( const weakpoints &set );
    static void load_weakpoint_sets( const JsonObject &jo, const std::string &src );
    static void reset();
    static void finalize_all();
    static const std::vector<weakpoints> &get_all();
};

#endif // CATA_SRC_WEAKPOINT_H
