#include "bodypart.h"
#include "translations.h"
#include "rng.h"

std::map<std::string, body_part> body_parts;

std::string body_part_name (body_part bp)
{
    switch (bp) {
    case bp_head:
        return _("head");
    case bp_eyes:
        return _("eyes");
    case bp_mouth:
        return _("mouth");
    case bp_torso:
        return _("torso");
    case bp_arm_l:
        return  _("left arm");
    case bp_arm_r:
        return  _("right arm");
    case bp_hand_l:
        return _("left hand");
    case bp_hand_r:
        return _("right hand");
    case bp_leg_l:
        return _("left leg");
    case bp_leg_r:
        return _("right leg");
    case bp_foot_l:
        return _("left foot");
    case bp_foot_r:
        return _("left foot");
    default:
        return _("appendix");
    }
}

std::string encumb_text(body_part bp)
{
    switch (bp) {
    case bp_head:
        return "";
    case bp_eyes:
        return _("Ranged combat is hampered.");
    case bp_mouth:
        return _("Running is slowed.");
    case bp_torso:
        return _("Dodging and melee is hampered.");
    case bp_arm_l:
    case bp_arm_r:
        return _("Melee and ranged combat is hampered.");
    case bp_hand_l:
    case bp_hand_r:
        return _("Manual tasks are slowed.");
    case bp_leg_l:
    case bp_leg_r:
        return _("Running and swimming are slowed.");
    case bp_foot_l:
    case bp_foot_r:
        return _("Running is slowed.");
    default:
        return _("It's inflammed.");
    }
}

body_part random_body_part(bool main_parts_only)
{
    int rn = rng(0, 100);
    if (!main_parts_only) {
        if (rn == 0) {
            return bp_eyes;
        }
        if (rn <= 1) {
            return bp_mouth;
        }
        if (rn <= 7) {
            return bp_head;
        }
        if (rn <= 16) {
            return bp_leg_l;
        }
        if (rn <= 25) {
            return bp_leg_r;
        }
        if (rn <= 28) {
            return bp_foot_l;
        }
        if (rn <= 31) {
            return bp_foot_r;
        }
        if (rn <= 40) {
            return bp_arm_l;
        }
        if (rn <= 49) {
            return bp_arm_r;
        }
        if (rn <= 52) {
            return bp_hand_l;
        }
        if (rn <= 55) {
            return bp_hand_r;
        }
        return bp_torso;
    } else {
        if (rn <= 7) {
            return bp_head;
        }
        if (rn <= 31) {
            return bp_legs;
        }
        if (rn <= 55) {
            return bp_arms;
        }
        return bp_torso;
    }
}

void init_body_parts()
{
    body_parts["TORSO"] = bp_torso;
    body_parts["HEAD"]  = bp_head;
    body_parts["EYES"]  = bp_eyes;
    body_parts["MOUTH"] = bp_mouth;
    body_parts["ARMS"]  = bp_arms;
    body_parts["HANDS"] = bp_hands;
    body_parts["LEGS"]  = bp_legs;
    body_parts["FEET"]  = bp_feet;
}

std::string get_body_part_id(body_part bp)
{
    switch (bp) {
    case bp_head:
        return "HEAD";
    case bp_eyes:
        return "EYES";
    case bp_mouth:
        return "MOUTH";
    case bp_torso:
        return "TORSO";
    case bp_arms:
        return "ARMS";
    case bp_hands:
        return "HANDS";
    case bp_legs:
        return "LEGS";
    case bp_feet:
        return "FEET";
    default:
        throw std::string("bad body part: %d", bp);
    }
}


