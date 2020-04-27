#pragma once
#ifndef CATA_SRC_CRAFTING_GUI_H
#define CATA_SRC_CRAFTING_GUI_H

class recipe;
class JsonObject;

const recipe *select_crafting_recipe( int &batch_size );

void load_recipe_category( const JsonObject &jsobj );
void reset_recipe_categories();

#endif // CATA_SRC_CRAFTING_GUI_H
