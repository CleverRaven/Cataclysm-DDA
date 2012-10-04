#include "player.h"
#include "bionics.h"
#include "game.h"
#include "keypress.h"
#include <sstream>
#include <stdlib.h>

#if (defined _WIN32 || defined WINDOWS)
	#include "catacurse.h"
#else
	#include <curses.h>
#endif

void hit_message(game *g, std::string subject, std::string verb,
                 std::string target, int dam, bool crit);
void melee_practice(player &u, bool hit, bool unarmed, bool bashing,
                               bool cutting, bool stabbing);
int  attack_speed(player &u, bool missed);
int  stumble(player &u);
std::string melee_verb(technique_id tech, std::string your, player &p,
                       int bash_dam, int cut_dam, int stab_dam);

/* Melee Functions!
 * These all belong to class player.
 *
 * STATE QUERIES
 * bool is_armed() - True if we are armed with any weapon.
 * bool unarmed_attack() - True if we are NOT armed with any weapon, but still
 *  true if we're wielding a bionic weapon (at this point, just itm_bio_claws).
 *
 * HIT DETERMINATION
 * int base_to_hit() - The base number of sides we get in hit_roll().
 *                     Dexterity / 2 + sk_melee
 * int hit_roll() - The player's hit roll, to be compared to a monster's or
 *   player's dodge_roll().  This handles weapon bonuses, weapon-specific
 *   skills, torso encumberment penalties and drunken master bonuses.
 */

bool player::is_armed()
{
 return (weapon.typeId() != 0 && !weapon.is_style());
}

bool player::unarmed_attack()
{
 return (weapon.typeId() == 0 || weapon.is_style() ||
         weapon.has_flag(IF_UNARMED_WEAPON));
}

int player::base_to_hit(bool real_life, int stat)
{
 if (stat == -999)
  stat = (real_life ? dex_cur : dex_max);
 return 1 + int(stat / 2) + sklevel[sk_melee];
}

int player::hit_roll()
{
 int stat = dex_cur;
// Some martial arts use something else to determine hits!
 switch (weapon.typeId()) {
  case itm_style_tiger:
   stat = (str_cur * 2 + dex_cur) / 3;
   break;
  case itm_style_leopard:
   stat = (per_cur + int_cur + dex_cur * 2) / 4;
   break;
  case itm_style_snake:
   stat = (per_cur + dex_cur) / 2;
   break;
 }
 int numdice = base_to_hit(stat) + weapon.type->m_to_hit +
               disease_intensity(DI_ATTACK_BOOST);
 int sides = 10 - encumb(bp_torso);
 int best_bonus = 0;
 if (sides < 2)
  sides = 2;

// Are we unarmed?
 if (unarmed_attack()) {
  best_bonus = sklevel[sk_unarmed];
  if (sklevel[sk_unarmed] > 4)
   best_bonus += sklevel[sk_unarmed] - 4; // Extra bonus for high levels
 }

// Using a bashing weapon?
 if (weapon.is_bashing_weapon()) {
  int bash_bonus = int(sklevel[sk_bashing] / 3);
  if (bash_bonus > best_bonus)
   best_bonus = bash_bonus;
 }

// Using a cutting weapon?
 if (weapon.is_cutting_weapon()) {
  int cut_bonus = int(sklevel[sk_cutting] / 2);
  if (cut_bonus > best_bonus)
   best_bonus = cut_bonus;
 }

// Using a spear?
 if (weapon.has_flag(IF_SPEAR) || weapon.has_flag(IF_STAB)) {
  int stab_bonus = int(sklevel[sk_stabbing] / 2);
  if (stab_bonus > best_bonus)
   best_bonus = stab_bonus;
 }

 numdice += best_bonus; // Use whichever bonus is best.

// Drunken master makes us hit better
 if (has_trait(PF_DRUNKEN)) {
  if (unarmed_attack())
   numdice += int(disease_level(DI_DRUNK) / 300);
  else
   numdice += int(disease_level(DI_DRUNK) / 400);
 }

 if (numdice < 1) {
  numdice = 1;
  sides = 8 - encumb(bp_torso);
 }

 return dice(numdice, sides);
}

int player::hit_mon(game *g, monster *z, bool allow_grab) // defaults to true
{
 bool is_u = (this == &(g->u));	// Affects how we'll display messages
 if (is_u)
  z->add_effect(ME_HIT_BY_PLAYER, 100); // Flag as attacked by us
 int j;
 bool can_see = (is_u || g->u_see(posx, posy, j));

 std::string You  = (is_u ? "You"  : name);
 std::string Your = (is_u ? "Your" : name + "'s");
 std::string your = (is_u ? "your" : (male ? "his" : "her"));
 std::string verb = "hit";
 std::string target = "the " + z->name();

// If !allow_grab, then we already grabbed them--meaning their dodge is hampered
 int mondodge = (allow_grab ? z->dodge_roll() : z->dodge_roll() / 3);

 bool missed = (hit_roll() < mondodge ||
                one_in(4 + dex_cur + weapon.type->m_to_hit));

 int move_cost = attack_speed(*this, missed);

 if (missed) {
  int stumble_pen = stumble(*this);
  if (is_u) {	// Only display messages if this is the player
   if (weapon.has_technique(TEC_FEINT, this))
    g->add_msg("You feint.");
   else if (stumble_pen >= 60)
    g->add_msg("You miss and stumble with the momentum.");
   else if (stumble_pen >= 10)
    g->add_msg("You swing wildly and miss.");
   else
    g->add_msg("You miss.");
  }
  melee_practice(*this, false, unarmed_attack(),
                 weapon.is_bashing_weapon(), weapon.is_cutting_weapon(),
                 (weapon.has_flag(IF_SPEAR) || weapon.has_flag(IF_STAB)));
  move_cost += stumble_pen;
  if (weapon.has_technique(TEC_FEINT, this))
   move_cost = rng(move_cost / 3, move_cost);
  moves -= move_cost;
  return 0;
 }
 moves -= move_cost;

 bool critical_hit = scored_crit(mondodge);

 int bash_dam = roll_bash_damage(z, critical_hit);
 int cut_dam  = roll_cut_damage(z, critical_hit);
 int stab_dam = roll_stab_damage(z, critical_hit);

 int pain = 0; // Boost to pain; required for perform_technique

// Moves lost to getting your weapon stuck
 int stuck_penalty = roll_stuck_penalty(z, (stab_dam >= cut_dam));
 if (weapon.is_style())
  stuck_penalty = 0;

// Pick one or more special attacks
 technique_id technique = pick_technique(g, z, NULL, critical_hit, allow_grab);

// Handles effects as well; not done in melee_affect_*
 perform_technique(technique, g, z, NULL, bash_dam, cut_dam, stab_dam, pain);
 z->speed -= int(pain / 2);

// Mutation-based attacks
 perform_special_attacks(g, z, NULL, bash_dam, cut_dam, stab_dam);

// Handles speed penalties to monster & us, etc
 melee_special_effects(g, z, NULL, critical_hit, bash_dam, cut_dam, stab_dam);

// Make a rather quiet sound, to alert any nearby monsters
 if (weapon.typeId() != itm_style_ninjutsu) // Ninjutsu is silent!
  g->sound(posx, posy, 8, "");

 verb = melee_verb(technique, your, *this, bash_dam, cut_dam, stab_dam);

 int dam = bash_dam + (cut_dam > stab_dam ? cut_dam : stab_dam);

 hit_message(g, You.c_str(), verb.c_str(), target.c_str(), dam, critical_hit);

 bool bashing = (bash_dam >= 10 && !unarmed_attack());
 bool cutting = (cut_dam >= 10 && cut_dam >= stab_dam);
 bool stabbing = (stab_dam >= 10 && stab_dam >= cut_dam);
 melee_practice(*this, true, unarmed_attack(), bashing, cutting, stabbing);

 if (allow_grab && technique == TEC_GRAB) {
// Move our weapon to a temp slot, if it's not unarmed
  if (!unarmed_attack()) {
   item tmpweap = remove_weapon();
   dam += hit_mon(g, z, false); // False means a second grab isn't allowed
   weapon = tmpweap;
  } else
   dam += hit_mon(g, z, false); // False means a second grab isn't allowed
 }

 if (dam >= 5 && has_artifact_with(AEP_SAP_LIFE))
  healall( rng(dam / 10, dam / 5) );
 return dam;
}

void player::hit_player(game *g, player &p, bool allow_grab)
{
 int j;
 bool is_u = (this == &(g->u));	// Affects how we'll display messages
 bool can_see = (is_u || g->u_see(posx, posy, j));
 if (is_u && p.is_npc()) {
  npc* npcPtr = dynamic_cast<npc*>(&p);
  npcPtr->make_angry();
 }

 std::string You  = (is_u ? "You"  : name);
 std::string Your = (is_u ? "Your" : name + "'s");
 std::string your = (is_u ? "your" : (male ? "his" : "her"));
 std::string verb = "hit";

// Divide their dodge roll by 2 if this is a grab
 int target_dodge = (allow_grab ? p.dodge_roll(g) : p.dodge_roll(g) / 2);
 int hit_value = hit_roll() - target_dodge;
 bool missed = (hit_roll() <= 0);

 int move_cost = attack_speed(*this, missed);

 if (missed) {
  int stumble_pen = stumble(*this);
  if (is_u) {	// Only display messages if this is the player
   if (weapon.has_technique(TEC_FEINT, this))
    g->add_msg("You feint.");
   else if (stumble_pen >= 60)
    g->add_msg("You miss and stumble with the momentum.");
   else if (stumble_pen >= 10)
    g->add_msg("You swing wildly and miss.");
   else
    g->add_msg("You miss.");
  }
  melee_practice(*this, false, unarmed_attack(),
                 weapon.is_bashing_weapon(), weapon.is_cutting_weapon(),
                 (weapon.has_flag(IF_SPEAR) || weapon.has_flag(IF_STAB)));
  move_cost += stumble_pen;
  if (weapon.has_technique(TEC_FEINT, this))
   move_cost = rng(move_cost / 3, move_cost);
  moves -= move_cost;
  return;
 }
 moves -= move_cost;

 body_part bp_hit;
 int side = rng(0, 1);
 hit_value += rng(-10, 10);
 if (hit_value >= 30)
  bp_hit = bp_eyes;
 else if (hit_value >= 20)
  bp_hit = bp_head;
 else if (hit_value >= 10)
  bp_hit = bp_torso;
 else if (one_in(4))
  bp_hit = bp_legs;
 else
  bp_hit = bp_arms;

 std::string target = (p.is_npc() ? p.name + "'s " : "your ");
 target += body_part_name(bp_hit, side);

 bool critical_hit = scored_crit(target_dodge);

 int bash_dam = roll_bash_damage(NULL, critical_hit);
 int cut_dam  = roll_cut_damage(NULL, critical_hit);
 int stab_dam = roll_stab_damage(NULL, critical_hit);

 technique_id tech_def = p.pick_defensive_technique(g, NULL, this);
 p.perform_defensive_technique(tech_def, g, NULL, this, bp_hit, side,
                               bash_dam, cut_dam, stab_dam);

 if (bash_dam + cut_dam + stab_dam <= 0)
  return; // Defensive technique canceled our attack!

 if (critical_hit) // Crits cancel out Toad Style's armor boost
  p.rem_disease(DI_ARMOR_BOOST);

 int pain = 0; // Boost to pain; required for perform_technique

// Moves lost to getting your weapon stuck
 int stuck_penalty = roll_stuck_penalty(NULL, (stab_dam >= cut_dam));
 if (weapon.is_style())
  stuck_penalty = 0;

// Pick one or more special attacks
 technique_id technique = pick_technique(g, NULL, &p, critical_hit, allow_grab);

// Handles effects as well; not done in melee_affect_*
 perform_technique(technique, g, NULL, &p, bash_dam, cut_dam, stab_dam, pain);
 p.pain += pain;

// Mutation-based attacks
 perform_special_attacks(g, NULL, &p, bash_dam, cut_dam, stab_dam);

// Handles speed penalties to monster & us, etc
 melee_special_effects(g, NULL, &p, critical_hit, bash_dam, cut_dam, stab_dam);

// Make a rather quiet sound, to alert any nearby monsters
 if (weapon.typeId() != itm_style_ninjutsu) // Ninjutsu is silent!
  g->sound(posx, posy, 8, "");

 p.hit(g, bp_hit, side, bash_dam, (cut_dam > stab_dam ? cut_dam : stab_dam));

 verb = melee_verb(technique, your, *this, bash_dam, cut_dam, stab_dam);
 int dam = bash_dam + (cut_dam > stab_dam ? cut_dam : stab_dam);
 hit_message(g, You.c_str(), verb.c_str(), target.c_str(), dam, critical_hit);

 bool bashing = (bash_dam >= 10 && !unarmed_attack());
 bool cutting = (cut_dam >= 10 && cut_dam >= stab_dam);
 bool stabbing = (stab_dam >= 10 && stab_dam >= cut_dam);
 melee_practice(*this, true, unarmed_attack(), bashing, cutting, stabbing);

 if (dam >= 5 && has_artifact_with(AEP_SAP_LIFE))
  healall( rng(dam / 10, dam / 5) );

 if (allow_grab && technique == TEC_GRAB) {
// Move our weapon to a temp slot, if it's not unarmed
  if (p.weapon.has_technique(TEC_BREAK, &p) &&
      dice(p.dex_cur + p.sklevel[sk_melee], 12) >
      dice(dex_cur + sklevel[sk_melee], 10)) {
   if (is_u)
    g->add_msg("%s break%s the grab!", target.c_str(), (p.is_npc() ? "s" : ""));
  } else if (!unarmed_attack()) {
   item tmpweap = remove_weapon();
   hit_player(g, p, false); // False means a second grab isn't allowed
   weapon = tmpweap;
  } else
   hit_player(g, p, false); // False means a second grab isn't allowed
 }
 if (tech_def == TEC_COUNTER) {
  if (!p.is_npc())
   g->add_msg("Counter-attack!");
  p.hit_player(g, *this);
 }
}

int stumble(player &u)
{
 int stumble_pen = 2 * u.weapon.volume() + u.weapon.weight();
 if (u.has_trait(PF_DEFT))
  stumble_pen = int(stumble_pen * .3) - 10;
 if (stumble_pen < 0)
  stumble_pen = 0;
// TODO: Reflect high strength bonus in newcharacter.cpp
 if (stumble_pen > 0 && (u.str_cur >= 15 || u.dex_cur >= 21 ||
                         one_in(16 - u.str_cur) || one_in(22 - u.dex_cur)))
  stumble_pen = rng(0, stumble_pen);

 return stumble_pen;
}

bool player::scored_crit(int target_dodge)
{
 bool to_hit_crit = false, dex_crit = false, skill_crit = false;
 int num_crits = 0;

// Weapon to-hit roll
 int chance = 25;
 if (unarmed_attack()) { // Unarmed attack: 1/2 of unarmed skill is to-hit
  for (int i = 1; i <= int(sklevel[sk_unarmed] * .5); i++)
   chance += (50 / (2 + i));
 }
 if (weapon.type->m_to_hit > 0) {
  for (int i = 1; i <= weapon.type->m_to_hit; i++)
   chance += (50 / (2 + i));
 } else if (chance < 0) {
  for (int i = 0; i > weapon.type->m_to_hit; i--)
   chance /= 2;
 }
 if (rng(0, 99) < chance + 4 * disease_intensity(DI_ATTACK_BOOST))
  num_crits++;

// Dexterity to-hit roll
// ... except sometimes we don't use dexteiry!
 int stat = dex_cur;
// Some martial arts use something else to determine hits!
 switch (weapon.typeId()) {
  case itm_style_tiger:
   stat = (str_cur * 2 + dex_cur) / 3;
   break;
  case itm_style_leopard:
   stat = (per_cur + int_cur + dex_cur * 2) / 4;
   break;
  case itm_style_snake:
   stat = (per_cur + dex_cur) / 2;
   break;
 }
 chance = 25;
 if (stat > 8) {
  for (int i = 9; i <= stat; i++)
   chance += (21 - i); // 12, 11, 10...
 } else {
  int decrease = 5;
  for (int i = 7; i >= stat; i--) {
   chance -= decrease;
   if (i % 2 == 0)
    decrease--;
  }
 }
 if (rng(0, 99) < chance)
  num_crits++;

// Skill level roll
 int best_skill = 0;

 if (weapon.is_bashing_weapon() && sklevel[sk_bashing] > best_skill)
  best_skill = sklevel[sk_bashing];
 if (weapon.is_cutting_weapon() && sklevel[sk_cutting] > best_skill)
  best_skill = sklevel[sk_cutting];
 if ((weapon.has_flag(IF_SPEAR) || weapon.has_flag(IF_STAB)) &&
     sklevel[sk_stabbing] > best_skill)
  best_skill = sklevel[sk_stabbing];
 if (unarmed_attack() && sklevel[sk_unarmed] > best_skill)
  best_skill = sklevel[sk_unarmed];

 best_skill += int(sklevel[sk_melee] / 2.5);

 chance = 25;
 if (best_skill > 3) {
  for (int i = 3; i < best_skill; i++)
   chance += (50 / (2 + i));
 } else if (chance < 3) {
  for (int i = 3; i > best_skill; i--)
   chance /= 2;
 }
 if (rng(0, 99) < chance + 4 * disease_intensity(DI_ATTACK_BOOST))
  num_crits++;

 if (num_crits == 3)
  return true;
 else if (num_crits == 2)
  return (hit_roll() >= target_dodge * 1.5 && !one_in(4));

 return false;
}

int player::dodge(game *g)
{
 if (has_disease(DI_SLEEP) || has_disease(DI_LYING_DOWN))
  return 0;
 if (activity.type != ACT_NULL)
  return 0;
 int ret = 4 + (dex_cur / 2);
 ret += sklevel[sk_dodge];
 ret += disease_intensity(DI_DODGE_BOOST);
 ret -= (encumb(bp_legs) / 2) + encumb(bp_torso);
 ret += int(current_speed(g) / 150);
 if (has_trait(PF_TAIL_LONG))
  ret += 4;
 if (has_trait(PF_TAIL_FLUFFY))
  ret += 8;
 if (has_trait(PF_WHISKERS))
  ret += 1;
 if (has_trait(PF_WINGS_BAT))
  ret -= 3;
 if (str_max >= 16)
  ret--; // Penalty if we're hyuuge
 else if (str_max <= 5)
  ret++; // Bonus if we're small
 if (dodges_left <= 0) { // We already dodged this turn
  if (rng(1, sklevel[sk_dodge] + dex_cur + 15) <= sklevel[sk_dodge] + dex_cur)
   ret = rng(0, ret);
  else
   ret = 0;
 }
 dodges_left--;
// If we're over our cap, average it with our cap
 if (ret > int(dex_cur / 2) + sklevel[sk_dodge] * 2)
  ret = ( ret + int(dex_cur / 2) + sklevel[sk_dodge] * 2 ) / 2;
 return ret;
}

int player::dodge_roll(game *g)
{
 return dice(dodge(g), 6);
}

int player::base_damage(bool real_life, int stat)
{
 if (stat == -999)
  stat = (real_life ? str_cur : str_max);
 int dam = (real_life ? rng(0, stat / 2) : stat / 2);
// Bonus for statong characters
 if (stat > 10)
  dam += int((stat - 9) / 2);
// Big bonus for super-human characters
 if (stat > 20)
  dam += int((stat - 20) * 1.5);

 return dam;
}

int player::roll_bash_damage(monster *z, bool crit)
{
 int ret = 0;
 int stat = str_cur; // Which stat determines damage?
 int skill = sklevel[sk_bashing]; // Which skill determines damage?
 if (unarmed_attack())
  skill = sklevel[sk_unarmed];
 
 switch (weapon.typeId()) { // Some martial arts change which stat
  case itm_style_crane:
   stat = (dex_cur * 2 + str_cur) / 3;
   break;
  case itm_style_snake:
   stat = int(str_cur + per_cur) / 2;
   break;
  case itm_style_dragon:
   stat = int(str_cur + int_cur) / 2;
   break;
 }

 ret = base_damage(true, stat);

// Drunken Master damage bonuses
 if (has_trait(PF_DRUNKEN) && has_disease(DI_DRUNK)) {
// Remember, a single drink gives 600 levels of DI_DRUNK
  int mindrunk, maxdrunk;
  if (unarmed_attack()) {
   mindrunk = disease_level(DI_DRUNK) / 600;
   maxdrunk = disease_level(DI_DRUNK) / 250;
  } else {
   mindrunk = disease_level(DI_DRUNK) / 900;
   maxdrunk = disease_level(DI_DRUNK) / 400;
  }
  ret += rng(mindrunk, maxdrunk);
 }

 int bash_dam = int(stat / 2) + weapon.damage_bash(),
     bash_cap = 5 + stat + skill;

 if (unarmed_attack())
  bash_dam = rng(0, int(stat / 2) + sklevel[sk_unarmed]);

 if (crit) {
  bash_dam *= 1.5;
  bash_cap *= 2;
 }

 if (bash_dam > bash_cap)// Cap for weak characters
  bash_dam = (bash_cap * 3 + bash_dam) / 4;

 if (z != NULL && z->has_flag(MF_PLASTIC))
  bash_dam /= rng(2, 4);

 int bash_min = bash_dam / 4;

 bash_dam = rng(bash_min, bash_dam);
 
 if (bash_dam < skill + int(stat / 2))
  bash_dam = rng(bash_dam, skill + int(stat / 2));

 ret += bash_dam;

 ret += disease_intensity(DI_DAMAGE_BOOST);

// Finally, extra crit effects
 if (crit) {
  ret += int(stat / 2);
  ret += skill;
  if (z != NULL)
   ret -= z->armor_bash() / 2;
 } else if (z != NULL)
  ret -= z->armor_bash();

 return (ret < 0 ? 0 : ret);
}

int player::roll_cut_damage(monster *z, bool crit)
{
 if (weapon.has_flag(IF_SPEAR))
  return 0;  // Stabs, doesn't cut!
 int z_armor_cut = (z == NULL ? 0 : z->armor_cut() - sklevel[sk_cutting] / 2);

 if (crit)
  z_armor_cut /= 2;
 if (z_armor_cut < 0)
  z_armor_cut = 0;

 int ret = weapon.damage_cut() - z_armor_cut;

 if (unarmed_attack() && !wearing_something_on(bp_hands)) {
  if (has_trait(PF_CLAWS))
   ret += 6;
  if (has_trait(PF_TALONS))
   ret += 6 + (sklevel[sk_unarmed] > 8 ? 8 : sklevel[sk_unarmed]);
  if (has_trait(PF_SLIME_HANDS) && (z == NULL || !z->has_flag(MF_ACIDPROOF)))
   ret += rng(4, 6);
 }

 if (ret <= 0)
  return 0; // No negative damage!

// 80%, 88%, 96%, 104%, 112%, 116%, 120%, 124%, 128%, 132%
 if (sklevel[sk_cutting] <= 5)
  ret *= double( 0.8 + 0.08 * sklevel[sk_cutting] );
 else
  ret *= double( 0.92 + 0.04 * sklevel[sk_cutting] );

 if (crit)
  ret *= double( 1.0 + double(sklevel[sk_cutting] / 12) );

 return ret;
}

int player::roll_stab_damage(monster *z, bool crit)
{
 int ret = 0;
 int z_armor = (z == NULL ? 0 : z->armor_cut() - 3 * sklevel[sk_stabbing]);

 if (crit)
  z_armor /= 3;
 if (z_armor < 0)
  z_armor = 0;

 if (unarmed_attack() && !wearing_something_on(bp_hands)) {
  ret = 0 - z_armor;
  if (has_trait(PF_CLAWS))
   ret += 6;
  if (has_trait(PF_NAILS) && z_armor == 0)
   ret++;
  if (has_trait(PF_THORNS))
   ret += 4;
 } else if (weapon.has_flag(IF_SPEAR) || weapon.has_flag(IF_STAB))
  ret = int((weapon.damage_cut() - z_armor) / 4);
 else
  return 0; // Can't stab at all!

 if (z != NULL && z->speed > 100) { // Bonus against fast monsters
  int speed_min = (z->speed - 100) / 10, speed_max = (z->speed - 100) / 5;
  int speed_dam = rng(speed_min, speed_max);
  if (speed_dam > ret * 2)
   speed_dam = ret * 2;
  if (speed_dam > 0)
   ret += speed_dam;
 }

 if (ret <= 0)
  return 0; // No negative stabbing!

 if (crit) {
  int multiplier = double( 1.0 + double(sklevel[sk_stabbing] / 5) );
  if (multiplier > 2.5)
   multiplier = 2.5;
  ret *= multiplier;
 }

 return ret;
}

int player::roll_stuck_penalty(monster *z, bool stabbing)
{
 int ret = 0;
 int basharm = (z == NULL ? 6 : z->armor_bash()),
     cutarm  = (z == NULL ? 6 : z->armor_cut());
 if (stabbing)
  ret = weapon.damage_cut() * 3 + basharm * 3 + cutarm * 3 -
        dice(sklevel[sk_stabbing], 10);
 else
  ret = weapon.damage_cut() * 4 + basharm * 5 + cutarm * 4 -
        dice(sklevel[sk_cutting], 10);

 if (ret >= weapon.damage_cut() * 10)
  return weapon.damage_cut() * 10;
 return (ret < 0 ? 0 : ret);
}

technique_id player::pick_technique(game *g, monster *z, player *p,
                                    bool crit, bool allowgrab)
{
 if (z == NULL && p == NULL)
  return TEC_NULL;

 std::vector<technique_id> possible;
 bool downed = ((z != NULL && !z->has_effect(ME_DOWNED)) ||
                (p != NULL && !p->has_disease(DI_DOWNED))  );
 bool plastic = (z == NULL || !z->has_flag(MF_PLASTIC));
 bool mon = (z != NULL);
 int base_str_req = 0;
 if (z != NULL)
  base_str_req = z->type->size;
 else if (p != NULL)
  base_str_req = 1 + (2 + p->str_cur) / 4;

 if (allowgrab) { // Check if grabs AREN'T REALLY ALLOWED
  if (mon && z->has_flag(MF_PLASTIC))
   allowgrab = false;
 }

 if (crit) { // Some are crit-only

  if (weapon.has_technique(TEC_SWEEP, this) &&
      (z == NULL || !z->has_flag(MF_FLIES)) && !downed)
   possible.push_back(TEC_SWEEP);

  if (weapon.has_technique(TEC_PRECISE, this))
   possible.push_back(TEC_PRECISE);

  if (weapon.has_technique(TEC_BRUTAL, this) && !downed &&
      str_cur + sklevel[sk_melee] >= 4 + base_str_req)
   possible.push_back(TEC_BRUTAL);

 }

 if (possible.empty()) { // Use non-crits only if any crit-onlies aren't used

  if (weapon.has_technique(TEC_DISARM, this) && !mon &&
      p->weapon.typeId() != 0 && !p->weapon.has_flag(IF_UNARMED_WEAPON) &&
      dice(   dex_cur +    sklevel[sk_unarmed],  8) >
      dice(p->dex_cur + p->sklevel[sk_melee],   10))
   possible.push_back(TEC_DISARM);

  if (weapon.has_technique(TEC_GRAB, this) && allowgrab)
   possible.push_back(TEC_GRAB);

  if (weapon.has_technique(TEC_RAPID, this))
   possible.push_back(TEC_RAPID);

  if (weapon.has_technique(TEC_THROW, this) && !downed &&
      str_cur + sklevel[sk_melee] >= 4 + base_str_req * 4 + rng(-4, 4))
   possible.push_back(TEC_THROW);

  if (weapon.has_technique(TEC_WIDE, this)) { // Count monsters
   int enemy_count = 0;
   for (int x = posx - 1; x <= posx + 1; x++) {
    for (int y = posy - 1; y <= posy + 1; y++) {
     int mondex = g->mon_at(x, y);
     if (mondex != -1) {
      if (g->z[mondex].friendly == 0)
       enemy_count++;
      else
       enemy_count -= 2;
     }
     int npcdex = g->npc_at(x, y);
     if (npcdex != -1) {
      if (g->active_npc[npcdex].attitude == NPCATT_KILL)
       enemy_count++;
      else
       enemy_count -= 2;
     }
    }
   }
   if (enemy_count >= (possible.empty() ? 2 : 3)) {
    possible.push_back(TEC_WIDE);
   }
  }

 } // if (possible.empty())
  
 if (possible.empty())
  return TEC_NULL;

 possible.push_back(TEC_NULL); // Always a chance to not use any technique

 return possible[ rng(0, possible.size() - 1) ];
}

void player::perform_technique(technique_id technique, game *g, monster *z,
                               player *p, int &bash_dam, int &cut_dam,
                               int &stab_dam, int &pain)
{
 bool mon = (z != NULL);
 std::string You = (is_npc() ? name : "You");
 std::string target = (mon ? "the " + z->name() :
                       (p->is_npc() ? p->name : "you"));
 std::string s = (is_npc() ? "s" : "");
 int tarx = (mon ? z->posx : p->posx), tary = (mon ? z->posy : p->posy);

 int junk;
 bool u_see = (!is_npc() || g->u_see(posx, posy, junk));

 if (technique == TEC_RAPID) {
  moves += int( attack_speed(*this, false) / 2);
  return;
 }
 if (technique == TEC_BLOCK) {
  bash_dam *= .7;
  return;
 }
// The rest affect our target, and thus depend on z vs. p
 switch (technique) {

 case TEC_SWEEP:
  if (z != NULL && !z->has_flag(MF_FLIES)) {
   z->add_effect(ME_DOWNED, rng(1, 2));
   bash_dam += z->fall_damage();
  } else if (p != NULL && !p->weapon.typeId() == itm_style_judo) {
   p->add_disease(DI_DOWNED, rng(1, 2), g);
   bash_dam += 3;
  }
  break;

 case TEC_PRECISE:
  if (z != NULL)
   z->add_effect(ME_STUNNED, rng(1, 4));
  else if (p != NULL)
   p->add_disease(DI_STUNNED, rng(1, 2), g);
  pain += rng(5, 8);
  break;

 case TEC_BRUTAL:
  if (z != NULL) {
   z->add_effect(ME_STUNNED, 1);
   z->knock_back_from(g, posx, posy);
  } else if (p != NULL) {
   p->add_disease(DI_STUNNED, 1, g);
   p->knock_back_from(g, posy, posy);
  }
  break;

 case TEC_THROW:
// Throws are less predictable than brutal strikes.
// We knock them back from a tile adjacent to us!
  if (z != NULL) {
   z->add_effect(ME_DOWNED, rng(1, 2));
   z->knock_back_from(g, posx + rng(-1, 1), posy + rng(-1, 1));
  } else if (p != NULL) {
   p->knock_back_from(g, posx + rng(-1, 1), posy + rng(-1, 1));
   if (!p->weapon.typeId() == itm_style_judo)
    p->add_disease(DI_DOWNED, rng(1, 2), g);
  }
  break;

 case TEC_WIDE: {
  int count_hit = 0;
  for (int x = posx - 1; x <= posx + 1; x++) {
   for (int y = posy - 1; y <= posy + 1; y++) {
    if (x != tarx || y != tary) { // Don't double-hit our target
     int mondex = g->mon_at(x, y);
     if (mondex != -1 && hit_roll() >= rng(0, 5) + g->z[mondex].dodge_roll()) {
      count_hit++;
      int dam = roll_bash_damage(&(g->z[mondex]), false) +
                roll_cut_damage (&(g->z[mondex]), false);
      g->z[mondex].hurt(dam);
      if (u_see)
       g->add_msg("%s hit%s %s for %d damage!", You.c_str(), s.c_str(),
                                                target.c_str(), dam);
     }
     int npcdex = g->npc_at(x, y);
     if (npcdex != -1 &&
         hit_roll() >= rng(0, 5) + g->active_npc[npcdex].dodge_roll(g)) {
      count_hit++;
      int dam = roll_bash_damage(NULL, false);
      int cut = roll_cut_damage (NULL, false);
      g->active_npc[npcdex].hit(g, bp_legs, 3, dam, cut);
      if (u_see)
       g->add_msg("%s hit%s %s for %d damage!", You.c_str(), s.c_str(),
                  g->active_npc[npcdex].name.c_str(), dam + cut);
     }
    }
   }
  }
  if (!is_npc())
   g->add_msg("%d enemies hit!", count_hit);
 } break;

 case TEC_DISARM:
  g->m.add_item(p->posx, p->posy, p->remove_weapon());
  if (u_see)
   g->add_msg("%s disarm%s %s!", You.c_str(), s.c_str(), target.c_str());
  break;

 } // switch (tech)
}

technique_id player::pick_defensive_technique(game *g, monster *z, player *p)
{
 if (blocks_left == 0)
  return TEC_NULL;

 int foe_melee_skill = 0;
 if (z != NULL)
  foe_melee_skill = z->type->melee_skill;
 else if (p != NULL)
  foe_melee_skill = p->dex_cur + p->sklevel[sk_melee];

 int foe_dodge = 0;
 if (z != NULL)
  foe_dodge = z->dodge_roll();
 else if (p != NULL)
  foe_dodge = p->dodge_roll(g);

 int foe_size = 0;
 if (z)
  foe_size = 4 + z->type->size * 4;
 else if (p) {
  foe_size = 12;
  if (p->str_max <= 5)
   foe_size -= 3;
  if (p->str_max >= 12)
   foe_size += 3;
 }

 blocks_left--;
 if (weapon.has_technique(TEC_WBLOCK_3) &&
     dice(dex_cur + sklevel[sk_melee], 12) > dice(foe_melee_skill, 10))
  return TEC_WBLOCK_3;

 if (weapon.has_technique(TEC_WBLOCK_2) &&
     dice(dex_cur + sklevel[sk_melee], 6) > dice(foe_melee_skill, 10))
  return TEC_WBLOCK_2;
 
 if (weapon.has_technique(TEC_WBLOCK_1) &&
     dice(dex_cur + sklevel[sk_melee], 3) > dice(foe_melee_skill, 10))
  return TEC_WBLOCK_1;

 if (weapon.has_technique(TEC_DEF_DISARM, this) &&
     z == NULL && p->weapon.typeId() != 0 &&
     !p->weapon.has_flag(IF_UNARMED_WEAPON) &&
     dice(   dex_cur +    sklevel[sk_unarmed], 8) >
     dice(p->dex_cur + p->sklevel[sk_melee],  10))
  return TEC_DEF_DISARM;

 if (weapon.has_technique(TEC_DEF_THROW, this) && 
     str_cur + sklevel[sk_melee] >= foe_size + rng(-4, 4) &&
     hit_roll() > rng(1, 5) + foe_dodge && !one_in(3))
  return TEC_DEF_THROW;

 if (weapon.has_technique(TEC_COUNTER, this) &&
     hit_roll() > rng(1, 10) + foe_dodge && !one_in(3))
  return TEC_COUNTER;

 if (weapon.has_technique(TEC_BLOCK_LEGS, this) &&
     (hp_cur[hp_leg_l] >= 20 || hp_cur[hp_leg_r] >= 20) &&
     dice(dex_cur + sklevel[sk_unarmed] + sklevel[sk_melee], 13) >
     dice(8 + foe_melee_skill, 10))
  return TEC_BLOCK_LEGS;

 if (weapon.has_technique(TEC_BLOCK, this) &&
     (hp_cur[hp_arm_l] >= 20 || hp_cur[hp_arm_r] >= 20) &&
     dice(dex_cur + sklevel[sk_unarmed] + sklevel[sk_melee], 16) >
     dice(6 + foe_melee_skill, 10))
  return TEC_BLOCK;

 blocks_left++; // We didn't use any blocks, so give it back!
 return TEC_NULL;
}

void player::perform_defensive_technique(
  technique_id technique, game *g, monster *z, player *p,
  body_part &bp_hit, int &side, int &bash_dam, int &cut_dam, int &stab_dam)

{
 int junk;
 bool mon = (z != NULL);
 std::string You = (is_npc() ? name : "You");
 std::string your = (is_npc() ? (male ? "his" : "her") : "your");
 std::string target = (mon ? "the " + z->name() : p->name);
 bool u_see = (!is_npc() || g->u_see(posx, posy, junk));

 switch (technique) {
  case TEC_BLOCK:
  case TEC_BLOCK_LEGS: {
   if (technique == TEC_BLOCK) {
    bp_hit = bp_arms;
    if (hp_cur[hp_arm_l] >= hp_cur[hp_arm_r])
     side = 0;
    else
     side = 1;
   } else { // Blocking with our legs
    bp_hit = bp_legs;
    if (hp_cur[hp_leg_l] >= hp_cur[hp_leg_r])
     side = 0;
    else
     side = 1;
   }
   if (u_see)
    g->add_msg("%s block%s with %s %s.", You.c_str(), (is_npc() ? "s" : ""),
               your.c_str(), body_part_name(bp_hit, side).c_str());
   bash_dam *= .5;
   double reduction = 1.0;
// Special reductions for certain styles
   if (weapon.typeId() == itm_style_tai_chi)
    reduction -= double(0.08 * double(per_cur - 6));
   if (weapon.typeId() == itm_style_taekwando)
    reduction -= double(0.08 * double(str_cur - 6));
   if (reduction > 1.0)
    reduction = 1.0;
   if (reduction < 0.3)
    reduction = 0.3;

   bash_dam *= reduction;
  } break;

  case TEC_WBLOCK_1:
  case TEC_WBLOCK_2:
  case TEC_WBLOCK_3:
// TODO: Cause weapon damage
   bash_dam = 0;
   cut_dam = 0;
   stab_dam = 0;
   if (u_see)
    g->add_msg("%s block%s with %s %s.", You.c_str(), (is_npc() ? "s" : ""),
               your.c_str(), weapon.tname().c_str());

  case TEC_COUNTER:
   break; // Handled elsewhere

  case TEC_DEF_THROW:
   if (u_see)
    g->add_msg("%s throw%s %s!", You.c_str(), (is_npc() ? "s" : ""),
               target.c_str());
   bash_dam = 0;
   cut_dam  = 0;
   stab_dam = 0;
   if (mon) {
    z->add_effect(ME_DOWNED, rng(1, 2));
    z->knock_back_from(g, posx + rng(-1, 1), posy + rng(-1, 1));
   } else {
    p->add_disease(DI_DOWNED, rng(1, 2), g);
    p->knock_back_from(g, posx + rng(-1, 1), posy + rng(-1, 1));
   }
   break;

  case TEC_DEF_DISARM:
   g->m.add_item(p->posx, p->posy, p->remove_weapon());
// Re-roll damage, without our weapon
   bash_dam = p->roll_bash_damage(NULL, false);
   cut_dam  = p->roll_cut_damage(NULL, false);
   stab_dam = p->roll_stab_damage(NULL, false);
   if (u_see)
    g->add_msg("%s disarm%s %s!", You.c_str(), (is_npc() ? "s" : ""),
                                  target.c_str());
   break;

 } // switch (technique)
}

void player::perform_special_attacks(game *g, monster *z, player *p,
                                     int &bash_dam, int &cut_dam, int &stab_dam)
{
 bool can_poison = false;
 int bash_armor = (z == NULL ? 0 : z->armor_bash());
 int cut_armor  = (z == NULL ? 0 : z->armor_cut());
 std::vector<special_attack> special_attacks = mutation_attacks(z, p);

 for (int i = 0; i < special_attacks.size(); i++) {
  bool did_damage = false;
  if (special_attacks[i].bash > bash_armor) {
   bash_dam += special_attacks[i].bash;
   did_damage = true;
  }
  if (special_attacks[i].cut > cut_armor) {
   cut_dam += special_attacks[i].cut - cut_armor;
   did_damage = true;
  }
  if (special_attacks[i].stab > cut_armor * .8) {
   stab_dam += special_attacks[i].stab - cut_armor * .8;
   did_damage = true;
  }

  if (!can_poison && one_in(2) &&
      (special_attacks[i].cut > cut_armor ||
       special_attacks[i].stab > cut_armor * .8))
   can_poison = true;

  if (did_damage)
   g->add_msg( special_attacks[i].text.c_str() );
 }

 if (can_poison && has_trait(PF_POISONOUS)) {
  if (z != NULL) {
   if (!is_npc() && !z->has_effect(ME_POISONED))
    g->add_msg("You poison the %s!", z->name().c_str());
   z->add_effect(ME_POISONED, 6);
  } else if (p != NULL) {
   if (!is_npc() && !p->has_disease(DI_POISON))
    g->add_msg("You poison %s!", p->name.c_str());
   p->add_disease(DI_POISON, 6, g);
  }
 }
}

void player::melee_special_effects(game *g, monster *z, player *p, bool crit,
                                   int &bash_dam, int &cut_dam, int &stab_dam)
{
 if (z == NULL && p == NULL)
  return;
 bool mon = (z != NULL);
 int junk;
 bool is_u = (!is_npc());
 bool can_see = (is_u || g->u_see(posx, posy, junk));
 std::string You = (is_u ? "You" : name);
 std::string Your = (is_u ? "Your" : name + "'s");
 std::string your = (is_u ? "your" : name + "'s");
 std::string target = (mon ? "the " + z->name() :
                       (p->is_npc() ? p->name : "you"));
 std::string target_possessive = (mon ? "the " + z->name() + "'s" :
                                  (p->is_npc() ? p->name + "'s" : your));
 int tarposx = (mon ? z->posx : p->posx), tarposy = (mon ? z->posy : p->posy);

// Bashing effecs
 if (mon)
  z->moves -= rng(0, bash_dam * 2);
 else
  p->moves -= rng(0, bash_dam * 2);

// Bashing crit
 if (crit && !unarmed_attack()) {
  int turns_stunned = int(bash_dam / 20) + rng(0, int(sklevel[sk_bashing] / 2));
  if (turns_stunned > 6)
   turns_stunned = 6;
  if (turns_stunned > 0) {
   if (mon)
    z->add_effect(ME_STUNNED, turns_stunned);
   else
    p->add_disease(DI_STUNNED, 1 + turns_stunned / 2, g);
  }
 }

// Stabbing effects
 int stab_moves = rng(stab_dam / 2, stab_dam * 1.5);
 if (crit)
  stab_moves *= 1.5;
 if (stab_moves >= 150) {
  if (can_see)
   g->add_msg("%s force%s the %s to the ground!", You.c_str(),
              (is_u ? "" : "s"), target.c_str());
  if (mon) {
   z->add_effect(ME_DOWNED, 1);
   z->moves -= stab_moves / 2;
  } else {
   p->add_disease(DI_DOWNED, 1, g);
   p->moves -= stab_moves / 2;
  }
 } else if (mon)
  z->moves -= stab_moves;
 else
  p->moves -= stab_moves;

// Bonus attacks!
 bool shock_them = (has_bionic(bio_shock) && power_level >= 2 &&
                    unarmed_attack() && (!mon || !z->has_flag(MF_ELECTRIC)) &&
                    one_in(3));

 bool drain_them = (has_bionic(bio_heat_absorb) && power_level >= 1 &&
                    !is_armed() && (!mon || z->has_flag(MF_WARM)));

 if (drain_them)
  power_level--;
 drain_them &= one_in(2);	// Only works half the time

 if (shock_them) {
  power_level -= 2;
  int shock = rng(2, 5);
  if (mon) {
   z->hurt( shock * rng(1, 3) );
   z->moves -= shock * 180;
   if (can_see)
    g->add_msg("%s shock%s %s!", You.c_str(), (is_u ? "" : "s"),
               target.c_str());
  } else {
   p->hurt(g, bp_torso, 0, shock * rng(1, 3));
   p->moves -= shock * 80;
  }
 }

 if (drain_them) {
  charge_power(rng(0, 4));
  if (can_see)
   g->add_msg("%s drain%s %s body heat!", You.c_str(), (is_u ? "" : "s"),
               target_possessive.c_str());
  if (mon) {
   z->moves -= rng(80, 120);
   z->speed -= rng(4, 6);
  } else
   p->moves -= rng(80, 120);
 }

 bool conductive = !wearing_something_on(bp_hands) && weapon.conductive();
                   
 if (mon && z->has_flag(MF_ELECTRIC) && conductive) {
  hurtall(rng(0, 1));
  moves -= rng(0, 50);
  if (is_u)
   g->add_msg("Contact with the %s shocks you!", z->name().c_str());
 }

// Glass weapons shatter sometimes
 if (weapon.made_of(GLASS) &&
     rng(0, weapon.volume() + 8) < weapon.volume() + str_cur) {
  if (can_see)
   g->add_msg("%s %s shatters!", Your.c_str(), weapon.tname(g).c_str());
  g->sound(posx, posy, 16, "");
// Dump its contents on the ground
  for (int i = 0; i < weapon.contents.size(); i++)
   g->m.add_item(posx, posy, weapon.contents[i]);
  hit(g, bp_arms, 1, 0, rng(0, weapon.volume() * 2));// Take damage
  if (weapon.is_two_handed(this))// Hurt left arm too, if it was big
   hit(g, bp_arms, 0, 0, rng(0, weapon.volume()));
  cut_dam += rng(0, 5 + int(weapon.volume() * 1.5));// Hurt the monster extra
  remove_weapon();
 }

// Getting your weapon stuck
 int cutting_penalty = roll_stuck_penalty(z, stab_dam > cut_dam);
 if (weapon.has_flag(IF_MESSY)) { // e.g. chainsaws
  cutting_penalty /= 6; // Harder to get stuck
  for (int x = tarposx - 1; x <= tarposx + 1; x++) {
   for (int y = tarposy - 1; y <= tarposy + 1; y++) {
    if (!one_in(3)) {
     if (g->m.field_at(x, y).type == fd_blood &&
         g->m.field_at(x, y).density < 3)
      g->m.field_at(x, y).density++;
     else
      g->m.add_field(g, x, y, fd_blood, 1);
    }
   }
  }
 }
 if (!unarmed_attack() && cutting_penalty > dice(str_cur * 2, 20)) {
  if (is_u)
   g->add_msg("Your %s gets stuck in %s, pulling it out of your hands!",
              weapon.tname().c_str(), target.c_str());
  if (mon) {
   if (weapon.has_flag(IF_SPEAR) || weapon.has_flag(IF_STAB))
    z->speed *= .7;
   else
    z->speed *= .85;
   z->add_item(remove_weapon());
  } else
   g->m.add_item(posx, posy, remove_weapon());
 } else {
  if (mon && (cut_dam >= z->hp || stab_dam >= z->hp)) {
   cutting_penalty /= 2;
   cutting_penalty -= rng(sklevel[sk_cutting], sklevel[sk_cutting] * 2 + 2);
  }
  if (cutting_penalty > 0)
   moves -= cutting_penalty;
  if (cutting_penalty >= 50 && is_u)
   g->add_msg("Your %s gets stuck in %s, but you yank it free.",
              weapon.tname().c_str(), target.c_str());
  if (mon && (weapon.has_flag(IF_SPEAR) || weapon.has_flag(IF_STAB)))
   z->speed *= .9;
 }

// Finally, some special effects for martial arts
 switch (weapon.typeId()) {

  case itm_style_karate:
   dodges_left++;
   blocks_left += 2;
   break;

  case itm_style_aikido:
   bash_dam /= 2;
   break;

  case itm_style_capoeira:
   add_disease(DI_DODGE_BOOST, 2, g, 2);
   break;

  case itm_style_muay_thai:
   if ((mon && z->type->size >= MS_LARGE) || (!mon && p->str_max >= 12))
    bash_dam += rng((mon ? z->type->size : (p->str_max - 8) / 4),
                    3 * (mon ? z->type->size : (p->str_max - 8) / 4));
   break;

  case itm_style_tiger:
   add_disease(DI_DAMAGE_BOOST, 2, g, 2, 10);
   break;

  case itm_style_centipede:
   add_disease(DI_SPEED_BOOST, 2, g, 4, 40);
   break;

  case itm_style_venom_snake:
   if (has_disease(DI_VIPER_COMBO)) {
    if (disease_intensity(DI_VIPER_COMBO) == 1) {
     if (is_u)
      g->add_msg("Snakebite!");
     int dambuf = bash_dam;
     bash_dam = stab_dam;
     stab_dam = dambuf;
     add_disease(DI_VIPER_COMBO, 2, g, 1, 2); // Upgrade to Viper Strike
    } else if (disease_intensity(DI_VIPER_COMBO) == 2) {
     if (hp_cur[hp_arm_l] >= hp_max[hp_arm_l] * .75 &&
         hp_cur[hp_arm_r] >= hp_max[hp_arm_r] * .75   ) {
      if (is_u)
       g->add_msg("Viper STRIKE!");
      bash_dam *= 3;
     } else if (is_u)
      g->add_msg("Your injured arms prevent a viper strike!");
     rem_disease(DI_VIPER_COMBO);
    }
   } else if (crit) {
    if (is_u)
     g->add_msg("Tail whip!  Viper Combo Intiated!");
    bash_dam += 5;
    add_disease(DI_VIPER_COMBO, 2, g, 1, 2);
   }
   break;

  case itm_style_scorpion:
   if (crit) {
    if (!is_npc())
     g->add_msg("Stinger Strike!");
    if (mon) {
     z->add_effect(ME_STUNNED, 3);
     int zposx = z->posx, zposy = z->posy;
     z->knock_back_from(g, posx, posy);
     if (z->posx != zposx || z->posy != zposy)
      z->knock_back_from(g, posx, posy); // Knock a 2nd time if the first worked
    } else {
     p->add_disease(DI_STUNNED, 2, g);
     int pposx = p->posx, pposy = p->posy;
     p->knock_back_from(g, posx, posy);
     if (p->posx != pposx || p->posy != pposy)
      p->knock_back_from(g, posx, posy); // Knock a 2nd time if the first worked
    }
   }
   break;

  case itm_style_zui_quan:
   dodges_left = 50; // Basically, unlimited.
   break;

 }
}

std::vector<special_attack> player::mutation_attacks(monster *z, player *p)
{
 std::vector<special_attack> ret;

 if (z == NULL && p == NULL)
  return ret;

 bool mon = (z != NULL);
 bool is_u = (!is_npc());// Affects how we'll display messages
 std::string You  = (is_u ? "You"  : name);
 std::string Your = (is_u ? "Your" : name + "'s");
 std::string your = (is_u ? "your" : (male ? "his" : "her"));
 std::string target = (mon ? "the " + z->name() : p->name);

 std::stringstream text;

 if (has_trait(PF_FANGS) && !wearing_something_on(bp_mouth) &&
     one_in(20 - dex_cur - sklevel[sk_unarmed])) {
  special_attack tmp;
  text << You << " sink" << (is_u ? " " : "s ") << your << " fangs into " <<
          target << "!";
  tmp.text = text.str();
  tmp.stab = 20;
  ret.push_back(tmp);
 }

 if (has_trait(PF_MANDIBLES) && one_in(22 - dex_cur - sklevel[sk_unarmed])) {
  special_attack tmp;
  text << You << " slice" << (is_u ? " " : "s ") << target << " with " <<
          your << " mandibles!";
  tmp.text = text.str();
  tmp.cut = 12;
  ret.push_back(tmp);
 }

 if (has_trait(PF_BEAK) && one_in(15 - dex_cur - sklevel[sk_unarmed])) {
  special_attack tmp;
  text << You << " peck" << (is_u ? " " : "s ") << target << "!";
  tmp.text = text.str();
  tmp.stab = 15;
  ret.push_back(tmp);
 }
  
 if (has_trait(PF_HOOVES) && one_in(25 - dex_cur - 2 * sklevel[sk_unarmed])) {
  special_attack tmp;
  text << You << " kick" << (is_u ? " " : "s ") << target << " with " <<
          your << " hooves!";
  tmp.text = text.str();
  tmp.bash = str_cur * 3;
  if (tmp.bash > 40)
   tmp.bash = 40;
  ret.push_back(tmp);
 }

 if (has_trait(PF_HORNS) && one_in(20 - dex_cur - sklevel[sk_unarmed])) {
  special_attack tmp;
  text << You << " headbutt" << (is_u ? " " : "s ") << target << " with " <<
          your << " horns!";
  tmp.text = text.str();
  tmp.bash = 3;
  tmp.stab = 3;
  ret.push_back(tmp);
 }

 if (has_trait(PF_HORNS_CURLED) && one_in(20 - dex_cur - sklevel[sk_unarmed])) {
  special_attack tmp;
  text << You << " headbutt" << (is_u ? " " : "s ") << target << " with " <<
          your << " curled horns!";
  tmp.text = text.str();
  tmp.bash = 14;
  ret.push_back(tmp);
 }

 if (has_trait(PF_HORNS_POINTED) && one_in(22 - dex_cur - sklevel[sk_unarmed])){
  special_attack tmp;
  text << You << " stab" << (is_u ? " " : "s ") << target << " with " <<
          your << " pointed horns!";
  tmp.text = text.str();
  tmp.stab = 24;
  ret.push_back(tmp);
 }

 if (has_trait(PF_ANTLERS) && one_in(20 - dex_cur - sklevel[sk_unarmed])) {
  special_attack tmp;
  text << You << " butt" << (is_u ? " " : "s ") << target << " with " <<
          your << " antlers!";
  tmp.text = text.str();
  tmp.bash = 4;
  ret.push_back(tmp);
 }

 if (has_trait(PF_TAIL_STING) && one_in(3) && one_in(10 - dex_cur)) {
  special_attack tmp;
  text << You << " sting" << (is_u ? " " : "s ") << target << " with " <<
          your << " tail!";
  tmp.text = text.str();
  tmp.stab = 20;
  ret.push_back(tmp);
 }

 if (has_trait(PF_TAIL_CLUB) && one_in(3) && one_in(10 - dex_cur)) {
  special_attack tmp;
  text << You << " hit" << (is_u ? " " : "s ") << target << " with " <<
          your << " tail!";
  tmp.text = text.str();
  tmp.bash = 18;
  ret.push_back(tmp);
 }

 if (has_trait(PF_ARM_TENTACLES) || has_trait(PF_ARM_TENTACLES_4) ||
     has_trait(PF_ARM_TENTACLES_8)) {
  int num_attacks = 1;
  if (has_trait(PF_ARM_TENTACLES_4))
   num_attacks = 3;
  if (has_trait(PF_ARM_TENTACLES_8))
   num_attacks = 7;
  if (weapon.is_two_handed(this))
   num_attacks--;

  for (int i = 0; i < num_attacks; i++) {
   if (one_in(18 - dex_cur - sklevel[sk_unarmed])) {
    special_attack tmp;
    text.str("");
    text << You << " slap" << (is_u ? " " : "s ") << target << " with " <<
            your << " tentacle!";
    tmp.text = text.str();
    tmp.bash = str_cur / 2;
    ret.push_back(tmp);
   }
  }
 }

 return ret;
}

std::string melee_verb(technique_id tech, std::string your, player &p,
                       int bash_dam, int cut_dam, int stab_dam)
{
 std::string s = (p.is_npc() ? "s" : "");

 if (tech != TEC_NULL && p.weapon.is_style() &&
     p.weapon.style_data(tech).name != "")
  return p.weapon.style_data(tech).name + s;

 std::stringstream ret;

 switch (tech) {

  case TEC_SWEEP:
   ret << "sweep" << s << "" << s << " " << your << " " << p.weapon.tname() <<
          " at";
   break;

  case TEC_PRECISE:
   ret << "jab" << s << " " << your << " " << p.weapon.tname() << " at";
   break;

  case TEC_BRUTAL:
   ret << "slam" << s << " " << your << " " << p.weapon.tname() << " against";
   break;

  case TEC_GRAB:
   ret << "wrap" << s << " " << your << " " << p.weapon.tname() << " around";
   break;

  case TEC_WIDE:
   ret << "swing" << s << " " << your << " " << p.weapon.tname() << " wide at";
   break;

  case TEC_THROW:
   ret << "use" << s << " " << your << " " << p.weapon.tname() << " to toss";
   break;

  default: // No tech, so check our damage levels
   if (bash_dam >= cut_dam && bash_dam >= stab_dam) {
    if (bash_dam >= 30)
     return "clobber" + s;
    if (bash_dam >= 20)
     return "batter" + s;
    if (bash_dam >= 10)
     return "whack" + s;
    return "hit" + s;
   }
   if (cut_dam >= stab_dam) {
    if (cut_dam >= 30)
     return "hack" + s;
    if (cut_dam >= 20)
     return "slice" + s;
    if (cut_dam >= 10)
     return "cut" + s;
    return "nick" + s;
   }
// Only stab damage is left
   if (stab_dam >= 30)
    return "impale" + s;
   if (stab_dam >= 20)
    return "pierce" + s;
   if (stab_dam >= 10)
    return "stab" + s;
   return "poke" + s;
 } // switch (tech)

 return ret.str();
}
 

void hit_message(game *g, std::string subject, std::string verb,
                          std::string target, int dam, bool crit)
{
 if (dam <= 0)
  g->add_msg("%s %s %s but do%s no damage.", subject.c_str(), verb.c_str(),
             target.c_str(), (subject == "You" ? "" : "es"));
 else
  g->add_msg("%s%s %s %s for %d damage.", (crit ? "Critical! " : ""),
             subject.c_str(), verb.c_str(), target.c_str(), dam);
}

void melee_practice(player &u, bool hit, bool unarmed, bool bashing,
                               bool cutting, bool stabbing)
{
 if (!hit) {
  u.practice(sk_melee, rng(2, 5));
  if (unarmed)
   u.practice(sk_unarmed, 2);
  if (bashing)
   u.practice(sk_bashing, 2);
  if (cutting)
   u.practice(sk_cutting, 2);
  if (stabbing)
   u.practice(sk_stabbing, 2);
 } else {
  u.practice(sk_melee, rng(5, 10));
  if (unarmed)
   u.practice(sk_unarmed, rng(5, 10));
  if (bashing)
   u.practice(sk_bashing, rng(5, 10));
  if (cutting)
   u.practice(sk_cutting, rng(5, 10));
  if (stabbing)
   u.practice(sk_stabbing, rng(5, 10));
 }
}

int attack_speed(player &u, bool missed)
{
 int move_cost = u.weapon.attack_time() + 20 * u.encumb(bp_torso);
 if (u.has_trait(PF_LIGHT_BONES))
  move_cost *= .9;
 if (u.has_trait(PF_HOLLOW_BONES))
  move_cost *= .8;

 move_cost -= u.disease_intensity(DI_SPEED_BOOST);

 if (move_cost < 25)
  return 25;

 return move_cost;
}
