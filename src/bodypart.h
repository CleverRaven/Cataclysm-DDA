#ifndef _BODYPART_H_
#define _BODYPART_H_

#include <string>
// The order is important ; pldata.h has to be in the same order
enum body_part {
 bp_torso = 0,
 bp_head,
 bp_eyes,
 bp_mouth,
 bp_arms,
 bp_hands,
 bp_legs,
 bp_feet,
 num_bp
};

std::string body_part_name(body_part bp, int side);
std::string encumb_text(body_part bp);

body_part random_body_part(bool main_parts_only = false);
int random_side(body_part);

#endif
