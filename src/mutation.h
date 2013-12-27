#ifndef _MUTATION_H_
#define _MUTATION_H_

#include "pldata.h"
#include "json.h"
#include "enums.h" // tripoint

#include <vector>
#include <map>

struct dream;
struct mutation_branch;

extern std::vector<dream> dreams;
extern std::map<std::string, std::vector<std::string> > mutations_category;
extern std::map<std::string, mutation_branch> mutation_data;
typedef std::pair<body_part, tripoint> mutation_wet;

struct dream
{
    std::vector<std::string> messages; // The messages that the dream will give
    std::string category; // The category that will trigger the dream
    int strength; // The category strength required for the dream

    dream() {
        category = "";
        strength = 0;
    }
};

struct mutation_branch
{
    bool valid; // True if this is a valid mutation (only used for starting traits)
    bool purifiable; // True if Purifier can remove it (False for *Special* mutations)
    std::vector<std::string> prereqs; // Prerequisites; Only one is required
    std::vector<std::string> cancels; // Mutations that conflict with this one
    std::vector<std::string> replacements; // Mutations that replace this one
    std::vector<std::string> additions; // Mutations that add to this one
    std::vector<std::string> category; // Mutation Categorys
    std::map<std::string,mutation_wet> protection; // Mutation wet effects

    mutation_branch() { valid = false; };
};

void init_mutation_parts();
void load_mutation(JsonObject &jsobj);
void load_dream(JsonObject &jsobj);

#endif
