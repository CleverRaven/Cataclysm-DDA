#pragma once
#ifndef MUTATION_H
#define MUTATION_H

#include "json.h"
#include "enums.h" // tripoint
#include "bodypart.h"
#include "color.h"
#include "damage.h"
#include "string_id.h"
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

class vitamin;
using vitamin_id = string_id<vitamin>;
class martialart;
using matype_id = string_id<martialart>;
struct dream;
struct mutation_branch;
class item;

extern std::vector<dream> dreams;
extern std::map<std::string, std::vector<std::string> > mutations_category;

struct dream {
    std::vector<std::string> messages; // The messages that the dream will give
    std::string category; // The category that will trigger the dream
    int strength; // The category strength required for the dream

    dream() {
        category = "";
        strength = 0;
    }
};

struct mut_attack {
    /** Text printed when the attack is proced by you */
    std::string attack_text_u;
    /** As above, but for npc */
    std::string attack_text_npc;
    /** Need all of those to qualify for this attack */
    std::set<std::string> required_mutations;
    /** Need none of those to qualify for this attack */
    std::set<std::string> blocker_mutations;

    /** If not num_bp, this body part needs to be uncovered for the attack to proc */
    body_part bp = num_bp;

    /** Chance to proc is one_in( chance - dex - unarmed ) */
    int chance = 0;

    damage_instance base_damage;
    /** Multiplied by strength and added to the above */
    damage_instance strength_damage;

    /** Should be true when and only when this attack needs hardcoded handling */
    bool hardcoded_effect = false;
};

struct mutation_branch {
    using MutationMap = std::unordered_map<std::string, mutation_branch>;
    // True if this is a valid mutation (False for "unavailable from generic mutagen").
    bool valid = false;
    // True if Purifier can remove it (False for *Special* mutations).
    bool purifiable;
    // True if it's a threshold itself, and shouldn't be obtained *easily* (False by default).
    bool threshold;
    // True if this is a trait associated with professional training/experience, so profession/quest ONLY.
    bool profession;
    // Wheather it has positive as well as negative effects.
    bool mixed_effect  = false;
    bool startingtrait = false;
    bool activated     = false;
    // Should it activate as soon as it is gained?
    bool starts_active = false;
    // Should it destroy gear on restricted body parts? (otherwise just pushes it off)
    bool destroys_gear = false;
    // Allow soft (fabric) gear on restricted body parts
    bool allow_soft_gear  = false;
    // IF any of the three are true, it drains that as the "cost"
    bool fatigue       = false;
    bool hunger        = false;
    bool thirst        = false;
    // How many points it costs in character creation
    int points     = 0;
    int visibility = 0;
    int ugliness   = 0;
    int cost       = 0;
    // costs are consumed consumed every cooldown turns,
    int cooldown   = 0;
    // bodytemp elements:
    int bodytemp_min = 0;
    int bodytemp_max = 0;
    int bodytemp_sleep = 0;
    // Healing per turn
    float healing_awake = 0.0f;
    float healing_resting = 0.0f;
    // Bonus HP multiplier. That is, 1.0 doubles hp, -0.5 halves it.
    float hp_modifier = 0.0f;
    // Second HP modifier that stacks with first but is otherwise identical.
    float hp_modifier_secondary = 0.0f;
    // Flat bonus/penalty to hp.
    float hp_adjustment = 0.0f;

    // Extra metabolism rate multiplier. 1.0 doubles usage, -0.5 halves.
    float metabolism_modifier = 0.0f;
    // As above but for thirst.
    float thirst_modifier = 0.0f;
    // As above but for fatigue.
    float fatigue_modifier = 0.0f;
    // Modifier for the rate at which fatigue drops when resting.
    float fatigue_regen_modifier = 0.0f;

    /** Attacks granted by this mutation */
    std::vector<mut_attack> attacks_granted;

    /** Mutations may adjust one or more of the default vitamin usage rates */
    std::map<vitamin_id, int> vitamin_rates;

    std::vector<std::string> prereqs; // Prerequisites; Only one is required
    std::vector<std::string> prereqs2; // Prerequisites; need one from here too
    std::vector<std::string> threshreq; // Prerequisites; dedicated slot to needing thresholds
    std::vector<std::string> cancels; // Mutations that conflict with this one
    std::vector<std::string> replacements; // Mutations that replace this one
    std::vector<std::string> additions; // Mutations that add to this one
    std::vector<std::string> category; // Mutation Categories
    std::set<std::string> flags; // Mutation flags
    std::map<body_part, tripoint> protection; // Mutation wet effects
    std::map<body_part, int> encumbrance_always; // Mutation encumbrance that always applies
    // Mutation encumbrance that applies when covered with unfitting item
    std::map<body_part, int> encumbrance_covered;
    // Body parts that now need OVERSIZE gear
    std::set<body_part> restricts_gear;
    /** Key pair is <active: bool, mod type: "STR"> */
    std::unordered_map<std::pair<bool, std::string>, int> mods; // Mutation stat mods
    std::map<body_part, resistances> armor;
    std::vector<matype_id>
    initial_ma_styles; // Martial art styles that can be chosen upon character generation
    std::string name;
    std::string description;
    /**
     * Returns the color to display the mutation name with.
     */
    nc_color get_display_color() const;
    /**
     * Returns true if a character with this mutation shouldn't be able to wear given item.
     */
    bool conflicts_with_item( const item &it ) const;
    /**
     * Returns damage resistance on a given body part granted by this mutation.
     */
    const resistances &damage_resistance( body_part bp ) const;
    /**
     * Check whether the given id is a valid mutation id (refers to a known mutation).
     */
    static bool has( const std::string &mutation_id );
    /**
     * Get the mutation data of a given mutation id. The id *must* be valid.
     */
    static const mutation_branch &get( const std::string &mutation_id );
    /**
     * Shortcut for getting the name of a (translated) mutation, same as
     * @code get( mutation_id ).name @endcode
     */
    static const std::string &get_name( const std::string &mutation_id );
    /**
     * All known mutations. Key is the mutation id, value is the mutation_branch that you would
     * also get by calling @ref get.
     */
    static const MutationMap &get_all();
    // For init.cpp: reset (clear) the mutation data
    static void reset_all();
    // For init.cpp: load mutation data from json
    static void load( JsonObject &jsobj );
    // For init.cpp: check internal consistency (valid ids etc.) of all mutations
    static void check_consistency();
};

void load_mutation_category( JsonObject &jsobj );
void load_dream( JsonObject &jsobj );

#endif
