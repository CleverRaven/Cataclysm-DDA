#include "bodypart.h"
#include "translations.h"
#include "rng.h"

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

body_part random_body_part()
{
 int rn = rng(0, 100);
 if (rn == 0)
  return bp_eyes;
 if (rn <= 1)
  return bp_mouth;
 if (rn <= 7)
  return bp_head;
 if (rn <= 28)
  return bp_legs;
 if (rn <= 50)
  return bp_arms;
 return bp_torso;
}
