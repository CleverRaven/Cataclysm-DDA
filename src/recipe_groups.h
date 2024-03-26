#pragma once
#ifndef CATA_SRC_RECIPE_GROUPS_H
#define CATA_SRC_RECIPE_GROUPS_H

#include <iosfwd>
#include <map>

#include "type_id.h"
#include "mapgendata.h"

class JsonObject;
class translation;

namespace recipe_group
{

void load( const JsonObject &jo, const std::string &src );
void check();
void reset();

std::map<recipe_id, translation> get_recipes_by_bldg( const std::string &bldg );
std::map<recipe_id, translation> get_recipes_by_id( const std::string &id, const std::optional<mapgen_arguments> *maybe_args,
        const std::optional<oter_id> &omt_ter = std::nullopt );
std::string get_building_of_recipe( const std::string &recipe );
} // namespace recipe_group

#endif // CATA_SRC_RECIPE_GROUPS_H
