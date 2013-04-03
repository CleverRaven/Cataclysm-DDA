#ifndef _MUTATION_H_
#define _MUTATION_H_

#include "pldata.h"	// See pldata.h for mutations--they're actually pl_flags
#include <vector>

enum mutation_category
{
 MUTCAT_NULL = 0,
 MUTCAT_LIZARD,
 MUTCAT_BIRD,
 MUTCAT_FISH,
 MUTCAT_BEAST,
 MUTCAT_CATTLE,
 MUTCAT_INSECT,
 MUTCAT_PLANT,
 MUTCAT_SLIME,
 MUTCAT_TROGLO,
 MUTCAT_CEPHALOPOD,
 MUTCAT_SPIDER,
 MUTCAT_RAT,
 NUM_MUTATION_CATEGORIES
};

// mutations_from_category() defines the lists; see mutation_data.cpp
std::vector<pl_flag> mutations_from_category(mutation_category cat);

struct mutation_branch
{
 bool valid; // True if this is a valid mutation (only used for flags < PF_MAX)
 std::vector<pl_flag> prereqs; // Prerequisites; Only one is required
 std::vector<pl_flag> cancels; // Mutations that conflict with this one
 std::vector<pl_flag> replacements; // Mutations that replace this one
 std::vector<pl_flag> additions; // Mutations that add to this one

 mutation_branch() { valid = false; };

};


#endif
