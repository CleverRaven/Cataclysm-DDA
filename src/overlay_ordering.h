#pragma once
#ifndef OVERLAY_ORDERING_H
#define OVERLAY_ORDERING_H

#include <map>

#include "string_id.h"

class JsonObject;

extern std::map<std::string, int> base_mutation_overlay_ordering;
extern std::map<std::string, int> tileset_mutation_overlay_ordering;

void load_overlay_ordering( JsonObject &jsobj );
void load_overlay_ordering_into_array( JsonObject &jsobj, std::map<std::string, int> &orderarray );
int get_overlay_order_of_mutation( const std::string &mutation_id_string );
void reset_overlay_ordering();

#endif
