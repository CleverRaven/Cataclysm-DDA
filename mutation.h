#ifndef _MUTATION_H_
#define _MUTATION_H_

#include "pldata.h"
#include <vector>
#include <map>

struct dream
{
    std::vector<std::string> message;			// The messages that the dream will give
    std::string category;	// The category that will trigger the dream
    int strength;					// The category strength required for the dream

    dream() {
        category = "";
        strength = 0;
    }
};

extern std::vector<dream> dreams;
extern std::map<std::string, std::vector<std::string> > mutations_category;

struct mutation_branch
{
    bool valid; // True if this is a valid mutation (only used for starting traits)
    std::vector<std::string> prereqs; // Prerequisites; Only one is required
    std::vector<std::string> cancels; // Mutations that conflict with this one
    std::vector<std::string> replacements; // Mutations that replace this one
    std::vector<std::string> additions; // Mutations that add to this one
    std::vector<std::string> category; // Mutation Categorys

    mutation_branch() { valid = false; };
};


#endif
