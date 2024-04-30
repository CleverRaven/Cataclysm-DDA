#pragma once
#ifndef CATA_SRC_CRAFTING_GUI_H
#define CATA_SRC_CRAFTING_GUI_H

#include <string>
#include <utility>
#include <vector>

#include "type_id.h"

class Character;
class JsonObject;
class recipe;

/**
 * Open crafting menu where user selects who will craft what (the crafter & the recipe).
 *
 * @param crafter: who opened the menu - the initial crafter
 * @param filterstring: initial filter to execute
 *
 * Return: if recipe * is not nullptr, then Character * is not nullptr either.
 */
std::pair<Character *, const recipe *> select_crafter_and_crafting_recipe( int &batch_size_out,
        const recipe_id &goto_recipe, Character *crafter, std::string filterstring = "" );

void load_recipe_category( const JsonObject &jsobj );
void reset_recipe_categories();

// Returns nullptr if the category does not exist, or a pointer to its vector
// of subcategories it the category does exist
const std::vector<std::string> *subcategories_for_category( const std::string &category );

#endif // CATA_SRC_CRAFTING_GUI_H
