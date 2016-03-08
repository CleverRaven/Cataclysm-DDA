#include "bodypart.h"
#include "translations.h"
#include "rng.h"
#include "debug.h"
#include <unordered_map>
#include <sstream>

namespace {
    const std::unordered_map<std::string, body_part> &get_body_parts_map()
    {
        static const std::unordered_map<std::string, body_part> body_parts = {
            { "TORSO", bp_torso },
            { "HEAD", bp_head },
            { "EYES", bp_eyes },
            { "MOUTH", bp_mouth },
            { "ARM_L", bp_arm_l },
            { "ARM_R", bp_arm_r },
            { "HAND_L", bp_hand_l },
            { "HAND_R", bp_hand_r },
            { "LEG_L", bp_leg_l },
            { "LEG_R", bp_leg_r },
            { "FOOT_L", bp_foot_l },
            { "FOOT_R", bp_foot_r },
            { "NUM_BP", num_bp },
        };
        return body_parts;
    }
} // namespace

body_part get_body_part_token( const std::string &id )
{
    auto & map = get_body_parts_map();
    const auto iter = map.find( id );
    if( iter == map.end() ) {
        debugmsg( "invalid body part id %s", id.c_str() );
        return bp_torso;
    }
    return iter->second;
}

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
        return _("right foot");
    default:
        return _("appendix");
    }
}

std::string body_part_paired_name(body_part bp)
{
    switch (bp) {
    case bp_arm_l:
    case bp_arm_r:
        return  _("arms");
    case bp_hand_l:
    case bp_hand_r:
        return _("hands");
    case bp_leg_l:
    case bp_leg_r:
        return _("legs");
    case bp_foot_l:
    case bp_foot_r:
        return _("feet");
    default:
        return body_part_name(bp);
    }
}

//TODO: Add function 'body_part_paired_name_accusative'. Both names may be shorten a bit to something like 'body_part_name_accus'

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
        return _("It's inflamed.");
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

body_part mutate_to_main_part(body_part bp)
{
    switch (bp) {
    case bp_torso:
        return bp_torso;

    case bp_head:
    case bp_eyes:
    case bp_mouth:
        return bp_head;

    case bp_arm_l:
    case bp_hand_l:
        return bp_arm_l;

    case bp_arm_r:
    case bp_hand_r:
        return bp_arm_r;

    case bp_leg_l:
    case bp_foot_l:
        return bp_leg_l;

    case bp_leg_r:
    case bp_foot_r:
        return bp_leg_r;

    default:
        return num_bp;
    }
}

bool paired_body_parts(body_part bp1, body_part bp2)
{
    const int sum = int(bp1) + int(bp2);
    // Paired limbs must be adjacent in the enum to keep it working. Order of pairs doesn't matter.
    return sum == (bp_arm_l + bp_arm_r) || sum == (bp_hand_l + bp_hand_r) || sum == (bp_leg_l + bp_leg_r) || sum == (bp_foot_l + bp_foot_r);
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
        debugmsg( "bad body part: %d", bp );
        return "HEAD";
    }
}

std::string recite_body_parts(const std::bitset<num_bp> &body_parts)
{
    int cnt = body_parts.count();
    std::stringstream res;

    if ( cnt == 0 ) {
        return res.str();
    }
    for( int i = 0; i < num_bp; i++ ) {
        const body_part bp1 = body_part(i);
        if (!body_parts.test(bp1)) {
            continue;
        }
        const body_part bp2 = body_part(i+1);
        const bool paired = body_parts.test(bp2) && paired_body_parts(bp1, bp2);
        if (paired) {
            cnt-=2;
            i++;
        } else {
            cnt-=1;
        }
        if (res.rdbuf()->in_avail() != 0) { // Indicates that stream is not empty
            (cnt == 0) ? res << " " << _("and") << " " : res << ", ";
        }
        res << ((paired) ? body_part_paired_name(bp1) : body_part_name(bp1));
    }
    return res.str();
}
