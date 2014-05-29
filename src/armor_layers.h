#ifndef _ARMOR_LAYERS_H_
#define _ARMOR_LAYERS_H_
#include "item.h"
#include "output.h"
#include <string>

// Colors for each stage of the damage of clothing
nc_color dam_color[] = {c_green, c_ltgreen, c_yellow, c_magenta, c_ltred, c_red};

// Order for displaying the body parts
int bp_order[] = {bp_head, bp_eyes, bp_mouth, bp_torso, bp_arms, bp_hands, bp_legs, bp_feet};

void draw_background(WINDOW *w, int left_w, int width_description);
void draw_description_pane(WINDOW *win, item *worn_item);

std::vector<std::string> clothing_properties(item *worn_item, int width);
std::vector<std::string> clothing_flags_description(item *worn_item);

#endif
