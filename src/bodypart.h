#ifndef BODYPART_H
#define BODYPART_H

#include <string>

// The order is important ; pldata.h has to be in the same order
enum body_part : int {
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

enum class side : int {
    BOTH,
    LEFT,
    RIGHT
};

// map integers to body part enum
const constexpr body_part bp_aBodyPart[] = {
    bp_torso, bp_head, bp_eyes, bp_mouth,
    bp_arm_l, bp_arm_r, bp_hand_l, bp_hand_r,
    bp_leg_l, bp_leg_r, bp_foot_l, bp_foot_r
};

/** Returns the opposite side. */
side opposite_side( side s );

// identify the index of a body part's "other half", or itself if not
const size_t bp_aiOther[] = {0, 1, 2, 3, 5, 4, 7, 6, 9, 8, 11, 10};

/** Returns the matching name of the body_part token. */
std::string body_part_name( body_part bp );

/** Returns the matching accusative name of the body_part token, i.e. "Shrapnel hits your X".
 *  These are identical to body_part_name above in english, but not in some other languages. */
std::string body_part_name_accusative( body_part bp );

/** Returns the name of the body parts in a context where the name is used as
 * a heading or title e.g. "Left Arm". */
std::string body_part_name_as_heading( body_part bp, int number );

/** Returns the matching encumbrance text for a given body_part token. */
std::string encumb_text( body_part bp );

/** Returns a random body_part token. main_parts_only will limit it to arms, legs, torso, and head. */
body_part random_body_part( bool main_parts_only = false );

/** Returns the matching main body_part that corresponds to the input; i.e. returns bp_arm_l from bp_hand_l. */
body_part mutate_to_main_part( body_part bp );
/** Returns the opposite body part (limb on the other side) */
body_part opposite_body_part( body_part bp );

/** Returns the matching body_part key from the corresponding body_part token. */
std::string get_body_part_id( body_part bp );

/** Returns the matching body_part token from the corresponding body_part string. */
body_part get_body_part_token( const std::string &id );

#endif
