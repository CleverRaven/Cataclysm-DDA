#pragma once
#ifndef CATA_SRC_RECIPE_GROUPS_H
#define CATA_SRC_RECIPE_GROUPS_H

#include <cstddef>
#include <map>
#include <optional>
#include <string>

#include "type_id.h"

class JsonObject;
class translation;
struct mapgen_arguments;

namespace recipe_group
{

void load( const JsonObject &jo, const std::string &src );
void check();
void reset();

std::map<recipe_id, translation> get_recipes_by_bldg( const std::string &bldg );
std::map<recipe_id, translation> get_recipes_by_id( const std::string &id );
bool has_recipes_by_id( const std::string &id, const oter_id &omt_ter,
                        const std::optional<mapgen_arguments> *maybe_args );
std::map<recipe_id, translation> get_recipes_by_id( const std::string &id, const oter_id &omt_ter,
        const std::optional<mapgen_arguments> *maybe_args, size_t limit = 0 );
std::string get_building_of_recipe( const std::string &recipe );
} // namespace recipe_group

#endif // CATA_SRC_RECIPE_GROUPS_H
