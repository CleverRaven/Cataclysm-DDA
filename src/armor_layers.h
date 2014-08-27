#ifndef _ARMOR_LAYERS_H_
#define _ARMOR_LAYERS_H_
#include "item.h"
#include "output.h"
#include <string>

// Colors for each stage of the damage of clothing
nc_color dam_color[] = {c_green, c_ltgreen, c_yellow, c_magenta, c_ltred, c_red};

// Order for displaying the body parts
int bp_order[] = {bp_head, bp_eyes, bp_mouth, bp_torso, bp_arm_l, bp_arm_r,
                  bp_hand_l, bp_hand_r, bp_leg_l, bp_leg_r, bp_foot_l, bp_foot_r};

void draw_background(WINDOW *w, int sort_w);
void draw_description_pane(WINDOW *win, item *worn_item);

std::string clothing_layer(item *worn_item);
std::vector<std::string> clothing_properties(item *worn_item, int width);
std::vector<std::string> clothing_flags_description(item *worn_item);

#endif
