#ifndef CRAFTING_GUI_H
#define CRAFTING_GUI_H

#include <string>
#include <vector>
#include <map>
struct recipe;
class JsonObject;

const recipe *select_crafting_recipe( int &batch_size );

void load_recipe_category( JsonObject &jsobj );
void reset_recipe_categories();

#endif // CRAFT_GUI_H
