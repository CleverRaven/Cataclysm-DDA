#pragma once
#ifndef RECIPE_GROUPS_H
#define RECIPE_GROUPS_H

#include <string>
#include <map>

#include "type_id.h"

class JsonObject;
class translation;

namespace recipe_group
{

void load( JsonObject &jo, const std::string &src );
void check();
void reset();

std::map<recipe_id, translation> get_recipes_by_bldg( const std::string &bldg );
std::map<recipe_id, translation> get_recipes_by_id( const std::string &id,
        const std::string &om_terrain_id = "ANY" );
} // namespace recipe_group

#endif
