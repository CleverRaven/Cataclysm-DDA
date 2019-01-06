#pragma once
#ifndef RECIPE_GROUPS_H
#define RECIPE_GROUPS_H

#include <string>

#include "map.h"

class JsonObject;

namespace recipe_group
{

void load( JsonObject &jo, const std::string &src );
void check();
void reset();

std::map<std::string, std::string> get_recipes( std::string id );

}

#endif
