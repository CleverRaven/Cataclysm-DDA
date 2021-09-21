#pragma once
#ifndef CATA_SRC_WEAKPOINT_H
#define CATA_SRC_WEAKPOINT_H

#include <array>
#include <string>
#include <vector>

#include "damage.h"
#include "type_id.h"

class Creature;
class JsonArray;
class JsonObject;

// Information about an attack on a weak point.
struct weakpoint_attack {
    // The source of the attack.
    const Creature *source;

    // The weapon used to make the attack.
    const item *weapon;

    // Weather the attack is a melee attack.
    bool is_melee;

    // The Creature's skill in hitting weak points.
    float wp_skill;

    weakpoint_attack();
};

struct weakpoint {
    // ID of the weakpoint. Equal to the name, if not provided.
    std::string id;
    // Name of the weakpoint. Can be empty.
    std::string name;
    // Percent chance of hitting the weakpoint. Can be increased by skill.
    float coverage = 100.0f;
    // Multiplier for existing armor values. Defaults to 1.
    std::array<float, static_cast<int>( damage_type::NUM )> armor_mult;
    // Flat penalty to armor values. Applied after the multiplier.
    std::array<float, static_cast<int>( damage_type::NUM )> armor_penalty;
    // Damage multipliers. Applied after armor.
    std::array<float, static_cast<int>( damage_type::NUM )> damage_mult;
    // Difficulty to hit the weak point.
    float difficulty = -10.0f;

    weakpoint();
    // Apply the armor multipliers and offsets to a set of resistances.
    void apply_to( resistances &resistances ) const;
    // Apply the damage multiplers to a set of damage values.
    void apply_to( damage_instance &damage ) const;
    // Return the change of the creature hitting the weakpoint.
    float hit_chance( const weakpoint_attack &attack ) const;
    void load( const JsonObject &jo );
};

struct weakpoints {
    // List of weakpoints. Each weakpoint should have a unique id.
    std::vector<weakpoint> weakpoint_list;
    // Default weakpoint to return.
    weakpoint default_weakpoint;

    // Selects a weakpoint to hit.
    const weakpoint *select_weakpoint( const weakpoint_attack &attack ) const;

    void clear();
    void load( const JsonArray &ja );
    void remove( const JsonArray &ja );
};

#endif // CATA_SRC_WEAKPOINT_H