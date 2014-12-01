#ifndef BODYPART_H
#define BODYPART_H

#include <string>
#include <map>

// The order is important ; pldata.h has to be in the same order
enum body_part {
    bp_torso = 0,
    bp_head,
    bp_eyes,
    bp_mouth,
    bp_arm_l,
    bp_arm_r,
    bp_hand_l,
    bp_hand_r,
    bp_leg_l,
    bp_leg_r,
    bp_foot_l,
    bp_foot_r,
    num_bp
};

enum handedness {
    NONE,
    LEFT,
    RIGHT
};

std::string body_part_name(body_part bp);
std::string body_part_name_accusative(body_part bp);
std::string encumb_text(body_part bp);

body_part random_body_part(bool main_parts_only = false);
body_part mutate_to_main_part(body_part bp);

void init_body_parts();
extern std::map<std::string, body_part> body_parts;
std::string get_body_part_id(body_part bp);

#endif
