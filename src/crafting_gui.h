#pragma once
#ifndef CRAFTING_GUI_H
#define CRAFTING_GUI_H

struct tripoint;

class recipe;
class JsonObject;

void crafting_bills_display( const tripoint &loc );
const recipe *select_crafting_recipe( int &batch_size );

void load_recipe_category( const JsonObject &jsobj );
void reset_recipe_categories();

#endif // CRAFT_GUI_H
