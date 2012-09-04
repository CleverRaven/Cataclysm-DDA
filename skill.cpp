#include "skill.h"

std::string skill_name(int sk)
{
 switch (sk) {
 case sk_null:
  return "nothing";
 case sk_dodge:
  return "dodge";
 case sk_melee:
  return "melee";
 case sk_unarmed:
  return "unarmed combat";
 case sk_bashing:
  return "bashing weapons";
 case sk_cutting:
  return "cutting weapons";
 case sk_stabbing:
  return "piercing weapons";
 case sk_throw:
  return "throwing";
 case sk_gun:
  return "firearms";
 case sk_pistol:
  return "handguns";
 case sk_shotgun:
  return "shotguns";
 case sk_smg:
  return "submachine guns";
 case sk_rifle:
  return "rifles";
 case sk_archery:
  return "archery";
 case sk_launcher:
  return "grenade launcher";
 case sk_computer:
  return "computers";
 case sk_mechanics:
  return "mechanics";
 case sk_electronics:
  return "electronics";
 case sk_cooking:
  return "cooking";
 case sk_carpentry:
  return "carpentry";
 case sk_survival:
  return "survival";
 case sk_traps:
  return "traps";
 case sk_tailor:
  return "tailoring";
 case sk_firstaid:
  return "first aid";
 case sk_speech:
  return "speech";
 case sk_barter:
  return "barter";
 case sk_swimming:
  return "swimming";
 case sk_driving:
  return "driving";
 case num_skill_types:
  return "out of bounds";
 }
 return "the skill with no name";
}

std::string skill_description(int sk)
{
 switch (sk) {
 case sk_null:
  return "No skill at all.";
 case sk_dodge:
  return "\
Your skill at dodging, whether it be from an attack, a trap, or a natural\n\
occurrence.  It is also used when attempting to fall gracefully, or in other\n\
acrobatic feats.";
 case sk_melee:
  return "\
Your skill at hand-to-hand combat, both with and without a weapon. At low\n\
levels this merely increases your chance to hit, but at higher levels it also\n\
increases the damage done significantly.";
 case sk_unarmed:
  return "\
Your skill at combat without a weapon. While at low levels, unarmed combat is\n\
a good way to die quickly, those skilled in it can eventually use throws and\n\
crushing blows to quickly dispatch enemies.";
 case sk_bashing:
  return "\
Your skill at combat with weapons that club or bash your enemies. This\n\
includes everything from rocks to baseball bats, and even the butt of rifles.\n\
This increases damage, and at higher levels will improve your chance to hit.";
 case sk_cutting:
  return "\
Your skill at combat with weapons that cut or slice your enemies. At first\n\
this will simply slightly increase accuracy and damage; later on it will\n\
help you bypass armor and thick hides.";
 case sk_stabbing:
  return "\
Your skill at combat that involves piercing weapons.  This includes spears,\n\
which only have a stabbing attack, and things like knives, which can cut or\n\
pierce.  This increases your accuracy and chance of a deadly critical.";
 case sk_throw:
  return "\
Your skill at throwing items and getting them to go where you want. Increases\n\
both accuracy and, at higher levels, range.";
 case sk_gun:
  return "\
Your general skill at firearms. This increases your accuracy with any gun,\n\
but not as much as the skill associated with the gun in question.";
 case sk_pistol:
  return "\
Your skill with handguns. These have poor accuracy, but generally are fired\n\
and reloaded faster than other guns, and are good in close quarters.";
 case sk_shotgun:
  return "\
Your skill with shotguns. Shot is easy to hit with, and does massive damage,\n\
but has short range. Shotguns may also be loaded with slugs, which go further\n\
and do as much damage, but are not very accurate.";
 case sk_smg:
  return "\
Your skill with submachine guns and machine pistols. Halfway between a pistol\n\
and an assault rifle, these weapons fire and reload quickly, and may fire in\n\
bursts, but they are not very accurate.";
 case sk_rifle:
  return "\
Your skill with rifles, both assault rifles and semi-auto hunting rifles.\n\
These have terrific range and accuracy, but are very slow to fire and reload,\n\
and are not very good in close quarters.";
 case sk_archery:
  return "\
Your skill with all kinds of bows, from crossbows to handmade long bows.\n\
These are quiet, but require some amount of strength to use, and are not\n\
terribly accurate. They can only be deadly in skilled hands.";
 case sk_launcher:
  return "\
Your skill with grenade launchers, whether standalone or as a rifle mod.\n\
These launchers fire a variety of 40mm grenades with different uses,\n\
but they and their rounds are hard to find.";
 case sk_computer:
  return "\
Your skill with computers, both in terms of legitimate use, and hacking. This\n\
will help you use complex systems, and bypass their security.";
 case sk_mechanics:
  return "\
Your skill with repairs to mechanical systems, and the crafting of many items\n\
that use complex parts, but are not electronics.";
 case sk_electronics:
  return "\
Your skill with repairs to electrical systems, and the crafting of many items\n\
that use electrical parts.";
 case sk_cooking:
  return "\
Your skill in combining food ingredients to make other, tastier food items.\n\
This may also be used in mixing chemicals and other more esoteric tasks.";
 case sk_carpentry:
  return "\
Your general knowledge in construction.  This is used primarily when building\n\
terrain, via the * screen.";
 case sk_survival:
  return "\
Your skill in wilderness survival.  This helps you butcher meat from a corpse\n\
and craft various basic survival items.  High levels may allow you to remove\n\
poison from tainted meat. Also used for skinning corpses to acquire pelts.";
 case sk_traps:
  return "\
Your skill at setting and disarming traps safely and effectively. Note that\n\
this does not help you detect traps, or avoid their effects if you activate\n\
them.";
 case sk_tailor:
  return "\
Your skill at repairing clothing, and at higher levels, creating clothing\n\
from basic materials.  Generally a sewing kit is needed.  Also applies to\n\
other methods of clothing repair and textile crafting.";
 case sk_firstaid:
  return "\
Your skill at emergency medical treatment, such as that from first aid kits\n\
and bandages. High levels of this skill may help heal more HP. Some drugs\n\
may be more effective as well.";
 case sk_speech:
  return "\
Your skill at talking to people, convincing them of things, lying, and many\n\
others. Works best in conjunction with a high intelligence.";
 case sk_barter:
  return "\
Your skill at getting a good deal when trading with others. Very high levels\n\
might even see you getting freebies.";
 case sk_swimming:
  return "\
Your skill at swimming.  This affects speed, your ability to swim while\n\
wearing clothes or carrying weights, and in-water combat.";
 case sk_driving:
  return "\
Your skill at driving. This affects how well you can control a vehicle,\n\
as well as the penalty of shooting while driving.";
 case num_skill_types:
  return "out of bounds";
 default:
  return "What is this skill I don't even know!  BUG!";
 }
}

std::string skill_long_name(skill sk, int level)
{
 switch (sk) {
  case sk_null:
   return "Nothing";
  case sk_dodge:
   if (level == 0)
    return "Easily-Hit";
   if (level == 1)
    return "huh";
  case sk_melee:
  case sk_unarmed:
  case sk_bashing:
  case sk_cutting:
  case sk_stabbing:
  case sk_throw:
  case sk_gun:
  case sk_pistol:
  case sk_shotgun:
  case sk_smg:
  case sk_rifle:
  case sk_archery:
  case sk_launcher:
  case sk_mechanics:
  case sk_electronics:
  case sk_cooking:
  case sk_tailor:
  case sk_carpentry:
  case sk_firstaid:
  case sk_speech:
  case sk_barter:
  case sk_computer:
  case sk_survival:
  case sk_traps:
  case sk_swimming:
  case sk_driving:
   return "huh";
 }
 return "huh";
}


double price_adjustment(int barter_skill)
{
 switch (barter_skill) {
  case 0:  return 1.5;
  case 1:  return 1.4;
  case 2:  return 1.2;
  case 3:  return 1.0;
  case 4:  return 0.8;
  case 5:  return 0.6;
  case 6:  return 0.5;
  default: return double(int(100 * (.3 + 1 / barter_skill)) / 100);
 }
}
