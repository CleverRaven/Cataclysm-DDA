#ifndef MUTATION_H
#define MUTATION_H

#include "pldata.h"
#include "json.h"
#include "enums.h"
#include "ui.h"
#include <vector>
#include <map>
#include <unordered_map>

extern std::map<std::string, mutation_type> mutation_types;
extern std::vector<dream> dreams;
typedef std::pair<body_part, tripoint> mutation_wet;

enum mut_rating {
    m_good,     // A good mutation.
    m_neutral,  // A neutral or little effect mutation.
    m_bad,      // A bad mutation.
    m_mixed     // A mutation with both good and bad effects.
}

class mutation_type
{
        friend void load_mutation_type(JsonObject &jo);
        friend class mutation;
    public:
        // --------------- Constructors ---------------
        mutation_type();
        mutation_type(const mutation_type &rhs);
        
        // --------------- Helpers ---------------
        bool load_mod_data(JsonObject &jsobj, std::string member);
        bool load_cost_data(JsonObject &jsobj, std::string member);
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
        /** Prerequisites, only 1 is needed. */
        std::vector<std::string> prereqs;
        /** Secondary prerequisites, only 1 is needed. */
        std::vector<std::string> prereqs2;
        /** Threshold prerequisite, only 1 is needed. */
        std::vector<std::string> threshreq;
        /** Mutations that this mutation removes. */
        std::vector<std::string> cancels;
        /** Mutations that this mutation can mutate into. */
        std::vector<std::string> replacements;
        /** Mutations that this mutation makes more likely to occur. */
        std::vector<std::string> additions;
        /** What categories does this mutation fall into? */
        std::vector<std::string> category;
        /** Mutation wet protection effect. */
        std::map<std::string, mutation_wet> protection;
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
        mutation();
        mutation(const mutation &rhs);
    protected:
        // --------------- Values ---------------
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
