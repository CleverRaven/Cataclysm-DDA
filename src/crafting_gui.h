#pragma once
#ifndef CATA_SRC_CRAFTING_GUI_H
#define CATA_SRC_CRAFTING_GUI_H

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "type_id.h"

class Character;
class JsonObject;
class inventory;
class recipe;
class recipe_subset;

/**
 * Open crafting menu where user selects who will craft what (the crafter & the recipe).
 *
 * @param crafter: who opened the menu - the initial crafter
 * @param filterstring: initial filter to execute
 *
 * Return: if recipe * is not nullptr, then Character * is not nullptr either.
 */
std::pair<Character *, const recipe *> select_crafter_and_crafting_recipe( int &batch_size_out,
        const recipe_id &goto_recipe, Character *crafter, std::string filterstring = "",
        bool camp_crafting = false, inventory *inventory_override = nullptr );
std::pair<std::vector<const recipe *>, bool> recipes_from_cat( const recipe_subset
        &available_recipes, const crafting_category_id &cat, const std::string &subcat );

void load_recipe_category( const JsonObject &jsobj, const std::string &src );
void reset_recipe_categories();

// Returns nullptr if the category does not exist, or a pointer to its vector
// of subcategories it the category does exist
const std::vector<std::string> *subcategories_for_category( const std::string &category );

struct crafting_category {
    crafting_category_id id;
    bool was_loaded = false;

    bool is_hidden;
    bool is_practice;
    bool is_building;
    bool is_wildcard;
    std::vector<std::string> subcategories;

    void load( const JsonObject &jo, std::string_view src );
};

#endif // CATA_SRC_CRAFTING_GUI_H
