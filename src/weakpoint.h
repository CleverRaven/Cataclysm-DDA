#pragma once
#ifndef CATA_SRC_WEAKPOINT_H
#define CATA_SRC_WEAKPOINT_H

#include <array>
#include <string>
#include <vector>

#include "creature.h"
#include "damage.h"

class JsonArray;
class JsonObject;

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

    weakpoint();
    // Apply the armor multipliers and offsets to a set of resistances.
    void apply_to( resistances &resistances ) const;
    // Return the change of the creature hitting the weakpoint.
    float hit_chance( Creature *source ) const;
    void load( const JsonObject &jo );
};

struct weakpoints {
    // List of weakpoints. Each weakpoint should have a unique id.
    std::vector<weakpoint> weakpoint_list;
    // Default weakpoint to return.
    weakpoint default_weakpoint;

    // Selects a weakpoint to hit.
    const weakpoint *select_weakpoint( Creature *source ) const;

    void clear();
    void load( const JsonArray &ja );
};

#endif // CATA_SRC_WEAKPOINT_H