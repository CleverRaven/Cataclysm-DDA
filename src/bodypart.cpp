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

std::string body_part_name_accusative (body_part bp)
{
    switch (bp) {
    case bp_head:
        return pgettext("bodypart_accusative", "head");
    case bp_eyes:
        return pgettext("bodypart_accusative", "eyes");
    case bp_mouth:
        return pgettext("bodypart_accusative", "mouth");
    case bp_torso:
        return pgettext("bodypart_accusative", "torso");
    case bp_arm_l:
        return pgettext("bodypart_accusative", "left arm");
    case bp_arm_r:
        return pgettext("bodypart_accusative", "right arm");
    case bp_hand_l:
        return pgettext("bodypart_accusative", "left hand");
    case bp_hand_r:
        return pgettext("bodypart_accusative", "right hand");
    case bp_leg_l:
        return pgettext("bodypart_accusative", "left leg");
    case bp_leg_r:
        return pgettext("bodypart_accusative", "right leg");
    case bp_foot_l:
        return pgettext("bodypart_accusative", "left foot");
    case bp_foot_r:
        return pgettext("bodypart_accusative", "right foot");
    default:
        return pgettext("bodypart_accusative", "appendix");
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
        if (rn <= 19) {
            return bp_leg_l;
        }
        if (rn <= 31) {
            return bp_leg_r;
        }
        if (rn <= 43) {
            return bp_arm_l;
        }
        if (rn <= 55) {
            return bp_arm_r;
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
    body_parts["ARM_L"]  = bp_arm_l;
    body_parts["ARM_R"]  = bp_arm_r;
    body_parts["HAND_L"] = bp_hand_l;
    body_parts["HAND_R"] = bp_hand_r;
    body_parts["LEG_L"]  = bp_leg_l;
    body_parts["LEG_R"]  = bp_leg_r;
    body_parts["FOOT_L"]  = bp_foot_l;
    body_parts["FOOT_R"]  = bp_foot_r;
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
    case bp_arm_l:
        return "ARM_L";
    case bp_arm_r:
        return "ARM_R";
    case bp_hand_l:
        return "HAND_L";
    case bp_hand_r:
        return "HAND_R";
    case bp_leg_l:
        return "LEG_L";
    case bp_leg_r:
        return "LEG_R";
    case bp_foot_l:
        return "FOOT_L";
    case bp_foot_r:
        return "FOOT_R";
    default:
        throw std::string("bad body part: %d", bp);
    }
}
