#include "bodypart.h"
#include "rng.h"

std::string body_part_name (body_part bp, int side)
{
 switch (bp) {
 case bp_head:
  return "head";
 case bp_eyes:
  return "eyes";
 case bp_mouth:
  return "mouth";
 case bp_torso:
  return "torso";
 case bp_arms:
  if (side == 0)
   return "left arm";
  if (side == 1)
   return "right arm";
  return "arms";
 case bp_hands:
  if (side == 0)
   return "left hand";
  if (side == 1)
   return "right hand";
  return "hands";
 case bp_legs:
  if (side == 0)
   return "left leg";
  if (side == 1)
   return "right leg";
  return "legs";
 case bp_feet:
  if (side == 0)
   return "left foot";
  if (side == 1)
   return "right foot";
  return "feet";
 default:
  return "appendix";
 }
}

std::string encumb_text(body_part bp)
{
 switch (bp) {
  case bp_head:  return "";
  case bp_eyes:  return "Ranged combat is hampered.";
  case bp_mouth: return "Running is slowed.";
  case bp_torso: return "Dodging and melee is hampered.";
  case bp_arms:  return "Melee and ranged combat is hampered.";
  case bp_hands: return "Manual tasks are slowed.";
  case bp_legs:  return "Running and swimming are slowed.";
  case bp_feet:  return "Running is slowed.";
  default: return "It's inflammed.";
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
