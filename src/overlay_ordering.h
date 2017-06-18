#pragma once
#ifndef OVERLAY_ORDERING_H
#define OVERLAY_ORDERING_H

#include "json.h"

#include <map>
#include <string>

#include "string_id.h"

struct mutation_branch;
using trait_id = string_id<mutation_branch>;

extern std::map<trait_id, int> base_mutation_overlay_ordering;
extern std::map<trait_id, int> tileset_mutation_overlay_ordering;

void load_overlay_ordering( JsonObject &jsobj );
void load_overlay_ordering_into_array( JsonObject &jsobj, std::map<trait_id, int> &orderarray );
void reset_overlay_ordering();

#endif
