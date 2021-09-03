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
    // Name of the weakpoint.
    std::string name = "";
    // Probability of hitting the weakpoint. Can be increased by skill.
    float coverage = 0;
    // Multiplier for existing armor values. Defaults to 1.
    std::array<float, static_cast<int>( damage_type::NUM )> armor_mult;
    // Constant offset for multiplied armor values. Defaults to 0.
    std::array<float, static_cast<int>( damage_type::NUM )> armor_offset;

    weakpoint();
    // Apply the armor multipliers and offsets to a set of resistances.
    void apply_to(resistances& resistances);
    void load( const JsonObject &obj );
};

struct weakpoints {
    // List of weakpoints. Each weakpoint should have a unique name.
    std::vector<weakpoint> weakpoints;

    // Selects a weakpoint to hit.
    weakpoint &select_weakpoint( Creature */*source*/ ) const;

    weakpoints();
    // Adds a weakpoint, possibly overriding a weakpoint with the same name.
    void add_weakpoint(weakpoint* weakpoint);
    // Removes all weakpoints with the given name.
    void remove_weakpoint(std::string weakpoint_name);
    void load( const JsonArray &obj );
};
