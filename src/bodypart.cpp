#include "bodypart.h"
#include "translations.h"
#include "rng.h"

std::map<std::string,body_part> body_parts;

std::string body_part_name (body_part bp, int side)
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
 case bp_arms:
  if (side == 0)
   return _("left arm");
  if (side == 1)
   return _("right arm");
  return _("arms");
 case bp_hands:
  if (side == 0)
   return _("left hand");
  if (side == 1)
   return _("right hand");
  return _("hands");
 case bp_legs:
  if (side == 0)
   return _("left leg");
  if (side == 1)
   return _("right leg");
  return _("legs");
 case bp_feet:
  if (side == 0)
   return _("left foot");
  if (side == 1)
   return _("right foot");
  return _("feet");
 default:
  return _("appendix");
 }
}

std::string encumb_text(body_part bp)
{
 switch (bp) {
  case bp_head:  return "";
  case bp_eyes:  return _("Ranged combat is hampered.");
  case bp_mouth: return _("Running is slowed.");
  case bp_torso: return _("Dodging and melee is hampered.");
  case bp_arms:  return _("Melee and ranged combat is hampered.");
  case bp_hands: return _("Manual tasks are slowed.");
  case bp_legs:  return _("Running and swimming are slowed.");
  case bp_feet:  return _("Running is slowed.");
  default: return _("It's inflammed.");
 }
}

body_part random_body_part(bool main_parts_only)
{
    int rn = rng(0, 100);
    if (!main_parts_only) {
        if (rn == 0)
            return bp_eyes;
        if (rn <= 1)
            return bp_mouth;
        if (rn <= 7)
            return bp_head;
        if (rn <= 24)
            return bp_legs;
        if (rn <= 30)
            return bp_feet;
        if (rn <= 47)
            return bp_arms;
        if (rn <= 53)
            return bp_hands;
        return bp_torso;
    } else {
        if (rn <= 7)
            return bp_head;
        if (rn <= 30)
            return bp_legs;
        if (rn <= 53)
            return bp_arms;
        return bp_torso;
    }
}

int random_side(body_part bp)
{
    switch (bp) {
        case bp_torso:
        case bp_head:
        case bp_eyes:
        case bp_mouth:
            return -1;
        case bp_arms:
        case bp_hands:
        case bp_legs:
        case bp_feet:
            return rng(0, 1);
        default:
            return rng(0, 1);
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
    case bp_head:  return "HEAD";
    case bp_eyes:  return "EYES";
    case bp_mouth: return "MOUTH";
    case bp_torso: return "TORSO";
    case bp_arms:  return "ARMS";
    case bp_hands: return "HANDS";
    case bp_legs:  return "LEGS";
    case bp_feet:  return "FEET";
    default: throw std::string("bad body part: %d", bp);
    }
}


