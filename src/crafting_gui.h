#pragma once
#ifndef CATA_SRC_CRAFTING_GUI_H
#define CATA_SRC_CRAFTING_GUI_H

#include <iosfwd>
#include <string>
#include <vector>

#include "type_id.h"

class JsonObject;
class recipe;

const recipe *select_crafting_recipe( int &batch_size_out, recipe_id goto_recipe );

void load_recipe_category( const JsonObject &jsobj );
void reset_recipe_categories();

// Returns nullptr if the category does not exist, or a pointer to its vector
// of subcategories it the category does exist
const std::vector<std::string> *subcategories_for_category( const std::string &category );

#endif // CATA_SRC_CRAFTING_GUI_H
