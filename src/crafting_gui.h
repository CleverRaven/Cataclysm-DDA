#pragma once
#ifndef CRAFTING_GUI_H
#define CRAFTING_GUI_H

class recipe;
class JsonObject;

const recipe *select_crafting_recipe( int &batch_size );
static bool query_is_yes( std::string query );

void load_recipe_category( JsonObject &jsobj );
void reset_recipe_categories();

#endif // CRAFT_GUI_H
