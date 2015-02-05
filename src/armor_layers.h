#ifndef ARMOR_LAYERS_H
#define ARMOR_LAYERS_H

#include "output.h"
#include <string>

class item;

void draw_mid_pane(WINDOW *win, item *worn_item);

std::string clothing_layer(item *worn_item);
std::vector<std::string> clothing_properties(item *worn_item, int width);
std::vector<std::string> clothing_flags_description(item *worn_item);

#endif
