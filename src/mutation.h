#ifndef MUTATION_H
#define MUTATION_H

#include "pldata.h"
#include "json.h"
#include "enums.h"
#include "ui.h"
#include <vector>
#include <map>
#include <unordered_map>

extern std::map<std::string, mutation_type> mut_types;
extern std::vector<dream> dreams;
typedef std::pair<body_part, tripoint> mutation_wet;

/** Handles the "replaces" field for mutation_type. */
void finalize_mut_types();

enum mut_rating {
    mut_good,     // A good mutation.
    mut_bad,      // A bad mutation.
    mut_mixed     // A mutation with both good and bad effects.
    mut_neutral,  // A neutral or little effect mutation.
}

class mutation_type
{
        friend void load_mutation_type(JsonObject &jo);
        friend void finalize_mut_types();
        friend class mutation;
    public:
        // --------------- Helpers ---------------
        bool load_mod_data(JsonObject &jsobj, std::string member);
        bool load_cost_data(JsonObject &jsobj, std::string member);
        
        // --------------- Other Functions ---------------
        /** Retrieves a stat mod of a mutation. */
        int get_mod(std::string mut, std::string arg) const;
    protected:
        // --------------- Values ---------------
        /** Mutation id. */
        muttype_id id;
        /** Mutation in-game name. */
        std::string name;
        /** Mutation in-game description. */
        std::string description;
        /** Point cost at character creation. */
        int points;
        /** Is it good, bad, neutral, or mixed? */
        mut_rating rating;
        /** How visible the mutation is. */
        int visibility;
        /** How ugly the mutation is compared to base human standards. */
        int ugliness;
        /** False for mutations that can't be obtained through normal means. */
        bool valid;
        /** Can the mutation be purified? */
        bool purifiable;
        /** Is the mutation a threshold mutation? */
        bool threshold;
        /** Is the mutation a profession based mutation? */
        bool profession;
        /** Can the mutation be selected as a starting trait? */
        bool startingtrait;
        /** Can the mutation be activated? */
        bool activatable;
        /** Automatically reactivates over time once turned on, paying the price over time. */
        bool repeating;
        /** How long do we activate for? */
        int duration;
        /** Prerequisites, only 1 is needed. */
        std::vector<muttype_id> prereqs;
        /** Secondary prerequisites, only 1 is needed. */
        std::vector<muttype_id> prereqs2;
        /** Threshold prerequisite, only 1 is needed. */
        std::vector<muttype_id> threshreq;
        /** Mutations that this mutation removes. */
        std::vector<muttype_id> cancels;
        /** Mutations that this mutation can mutate into. */
        std::vector<muttype_id> replacements;
        /** Mutations that this mutation mutates from. */
        std::vector<muttype_id> replaces;
        /** Mutations that this mutation makes more likely to occur. */
        std::vector<muttype_id> additions;
        /** What categories does this mutation fall into? */
        std::vector<muttype_id> category;
        /** Mutation wet protection effect. */
        std::map<muttype_id, mutation_wet> protection;
        /** Martial art styles that can be chosen on character generation with this mutation.
         *  Used mainly by profession mutations. */
        std::vector<std::string> initial_ma_styles;
        /** Mutation stat modifiers, key pair order is <active: bool, mod type: "STR"> */
        std::unordered_map<std::pair<bool, std::string>, int> mods;
        /** Mutation activation costs. Key is the cost type, i.e. "fatigue" */
        std::unordered_map<std::string, int> costs;
}

class mutation : public JsonSerializer, public JsonDeserializer
{
    public:
        // --------------- Constructors ---------------
        mutation() :
            mut_type(NULL),
            charge(0),
            key('')
        { }
        mutation(mutation_type *pmut_type) :
            mut_type(pmut_type),
            charge(0),
            key('')
        { }
        mutation(const mutation &rhs);
        
        // --------------- Accessors ---------------
        char get_key() const;
        void set_key(char new_key);
        
        /** Returns the various costs of a mutation in the passed in map. */
        void get_costs(std::unordered_map<std::string, int> &cost);
        /** Returns the named cost of a mutation. */
        int get_cost(std::string arg);
        /** Returns the activation duration of a mutation. */
        int get_duration();
        /** Returns true if a mutation automatically reactivates. */
        bool get_repeating();
        
        
        // --------------- Other Functions ---------------
        /** Returns true if the mutation successfully activated. */
        bool activate(Character &ch);
        /** Returns true if the mutation is currently active. */
        bool is_active();
        
    protected:
        // --------------- Values ---------------
        /** What mutation type is this?. */
        mutation_type *mut_type const;
        /** How many charges the mutation currently has. */
        int charge;
        /** What key is the mutation bound to. */
        char key;
}

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

void load_dream(JsonObject &jsobj);

#endif
