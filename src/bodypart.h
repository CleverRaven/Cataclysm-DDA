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

/** Returns the matching name of the body_part token. */
std::string body_part_name(body_part bp);
/** Returns the matching accusative name of the body_part token, i.e. "Shrapnel hits your X".
 *  These are identical to body_part_name above in english, but not in some other languages. */
std::string body_part_name_accusative(body_part bp);
/** Returns the matching encumbrance text for a given body_part token. */
std::string encumb_text(body_part bp);

/** Returns a random body_part token. main_parts_only will limit it to arms, legs, torso, and head. */
body_part random_body_part(bool main_parts_only = false);
/** Returns the matching main body_part that corresponds to the input; i.e. returns bp_arm_l from bp_hand_l. */
body_part mutate_to_main_part(body_part bp);

/** Initializes the body_part token map keys; i.e. "HEAD" -> bp_head. */
void init_body_parts();
extern std::map<std::string, body_part> body_parts;
/** Returns the matching body_part key from the corresponding body_part token. */
std::string get_body_part_id(body_part bp);

#endif
