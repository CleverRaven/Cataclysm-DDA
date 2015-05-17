#ifndef MUTATION_H
#define MUTATION_H

#include "json.h"
#include "enums.h" // tripoint
#include "bodypart.h"
#include "color.h"
#include "string_id.h"
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

class martialart;
using matype_id = string_id<martialart>;
struct dream;
struct mutation_branch;

extern std::vector<dream> dreams;
extern std::map<std::string, std::vector<std::string> > mutations_category;
typedef std::pair<body_part, tripoint> mutation_wet;

struct dream {
    std::vector<std::string> messages; // The messages that the dream will give
    std::string category; // The category that will trigger the dream
    int strength; // The category strength required for the dream

    dream()
    {
        category = "";
        strength = 0;
    }
};

struct mutation_branch {
    using MutationMap = std::unordered_map<std::string, mutation_branch>;
    bool valid = false; // True if this is a valid mutation (False for "unavailable from generic mutagen")
    bool purifiable; // True if Purifier can remove it (False for *Special* mutations)
    bool threshold; // True if it's a threshold itself, and shouldn't be obtained *easily* (False by default)
    bool profession; // True if this is a trait associated with professional training/experience, so profession/quest ONLY
    bool mixed_effect  = false; // Wheather it has positive as well as negative effects.
    bool startingtrait = false; // Starting Trait True/False
    bool activated     = false;
    bool fatigue       = false; //IF any of the three are true, it drains that as the "cost"
    bool hunger        = false;
    bool thirst        = false;
    int points     = 0; // How many points it costs in character creation
    int visibility = 0; // How visible it is
    int ugliness   = 0; // How ugly it is
    int cost       = 0;
    int cooldown   = 0; // costs are consumed consumed every N turns,
    std::vector<std::string> prereqs; // Prerequisites; Only one is required
    std::vector<std::string> prereqs2; // Prerequisites; need one from here too
    std::vector<std::string> threshreq; // Prerequisites; dedicated slot to needing thresholds
    std::vector<std::string> cancels; // Mutations that conflict with this one
    std::vector<std::string> replacements; // Mutations that replace this one
    std::vector<std::string> additions; // Mutations that add to this one
    std::vector<std::string> category; // Mutation Categories
    std::map<std::string, mutation_wet> protection; // Mutation wet effects
    /** Key pair is <active: bool, mod type: "STR"> */
    std::unordered_map<std::pair<bool, std::string>, int> mods; // Mutation stat mods
    std::vector<matype_id> initial_ma_styles; // Martial art styles that can be chosen upon character generation
    std::string name;
    std::string description;
    /**
     * Returns the color to display the mutation name with.
     */
    nc_color get_display_color() const;
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

void load_mutation_category(JsonObject &jsobj);
void load_dream(JsonObject &jsobj);

#endif
