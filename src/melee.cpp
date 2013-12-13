#include "player.h"
#include "bionics.h"
#include "debug.h"
#include "game.h"
#include "keypress.h"
#include "martialarts.h"
#include <sstream>
#include <stdlib.h>
#include <algorithm>

#include "cursesdef.h"

void player_hit_message(game* g, player* attacker, std::string message,
                        std::string target_name, int dam, bool crit);
void melee_practice(const calendar& turn, player &u, bool hit, bool unarmed,
                    bool bashing, bool cutting, bool stabbing);
int  attack_speed(player &u);
int  stumble(player &u);
std::string melee_message(matec_id tech, player &p, int bash_dam, int cut_dam, int stab_dam);

/* Melee Functions!
 * These all belong to class player.
 *
 * STATE QUERIES
 * bool is_armed() - True if we are armed with any weapon.
 * bool unarmed_attack() - True if we are NOT armed with any weapon, but still
 *  true if we're wielding a bionic weapon (at this point, just "bio_claws").
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
 return (weapon.typeId() != "null");
}

bool player::unarmed_attack() {
 return (weapon.typeId() == "null" || weapon.has_flag("UNARMED_WEAPON"));
}

int player::base_to_hit(bool real_life, int stat)
{
 if (stat == -999)
  stat = (real_life ? dex_cur : dex_max);
 return 1 + int(stat / 2) + skillLevel("melee");
}

int player::hit_roll()
{
 int stat = dex_cur;
// apply martial arts bonuses
  stat += mabuff_tohit_bonus();


// keep the old martial arts mechanics for now
// Some martial arts use something else to determine hits!
 if(weapon.typeId() == "style_tiger"){
   stat = (str_cur * 2 + dex_cur) / 3;
 } else if(weapon.typeId() == "style_leopard"){
   stat = (per_cur + int_cur + dex_cur * 2) / 4;
 } else if(weapon.typeId() == "style_snake"){
   stat = (per_cur + dex_cur) / 2;
 }
 int numdice = base_to_hit(stat) + weapon.type->m_to_hit +
               disease_intensity("attack_boost");
 int sides = 10 - encumb(bp_torso);
 int best_bonus = 0;
 if (sides < 2)
  sides = 2;

// Are we unarmed?
 if (unarmed_attack()) {
  best_bonus = skillLevel("unarmed");
  if (skillLevel("unarmed") > 4)
   best_bonus += skillLevel("unarmed") - 4; // Extra bonus for high levels
 }

// Using a bashing weapon?
 if (weapon.is_bashing_weapon()) {
  int bash_bonus = int(skillLevel("bashing") / 3);
  if (bash_bonus > best_bonus)
   best_bonus = bash_bonus;
 }

// Using a cutting weapon?
 if (weapon.is_cutting_weapon()) {
  int cut_bonus = int(skillLevel("cutting") / 2);
  if (cut_bonus > best_bonus)
   best_bonus = cut_bonus;
 }

// Using a spear?
 if (weapon.has_flag("SPEAR") || weapon.has_flag("STAB")) {
  int stab_bonus = int(skillLevel("stabbing") / 2);
  if (stab_bonus > best_bonus)
   best_bonus = stab_bonus;
 }

 numdice += best_bonus; // Use whichever bonus is best.

// Drunken master makes us hit better
 if (has_trait("DRUNKEN")) {
  if (unarmed_attack())
   numdice += int(disease_duration("drunk") / 300);
  else
   numdice += int(disease_duration("drunk") / 400);
 }

// Farsightedness makes us hit worse
 if (has_trait("HYPEROPIC") && !is_wearing("glasses_reading")
     && !is_wearing("glasses_bifocal")) {
  numdice -= 2;
 }

 if (numdice < 1) {
  numdice = 1;
  sides = 8 - encumb(bp_torso);
 }

 return dice(numdice, sides);
}


int player::hit_mon(game *g, monster *critter, bool allow_grab) // defaults to true
{
    bool is_u = (this == &(g->u)); // Affects how we'll display messages
    if (is_u) {
        critter->add_effect(ME_HIT_BY_PLAYER, 100); // Flag as attacked by us
    }

    std::string message = is_u ? _("You hit %s") : _("<npcname> hits %s");
    std::string target_name = string_format(_("the %s"), critter->name().c_str());

// If !allow_grab, then we already grabbed them--meaning their dodge is hampered
 int mondodge = (allow_grab ? critter->dodge_roll() : critter->dodge_roll() / 3);

 bool missed = (hit_roll() < mondodge ||
                one_in(4 + dex_cur + weapon.type->m_to_hit));

 int move_cost = attack_speed(*this);

 if (missed) {
  int stumble_pen = stumble(*this);
  if (is_u) { // Only display messages if this is the player
   if (has_miss_recovery_tec())
    g->add_msg(_("You feint."));
   else if (stumble_pen >= 60)
    g->add_msg(_("You miss and stumble with the momentum."));
   else if (stumble_pen >= 10)
    g->add_msg(_("You swing wildly and miss."));
   else
    g->add_msg(_("You miss."));
  }
  melee_practice(g->turn, *this, false, unarmed_attack(),
                 weapon.is_bashing_weapon(), weapon.is_cutting_weapon(),
                 (weapon.has_flag("SPEAR") || weapon.has_flag("STAB")));
  move_cost += stumble_pen;
  if (has_miss_recovery_tec())
   move_cost = rng(move_cost / 3, move_cost);
  moves -= move_cost;
  return 0;
 }
 moves -= move_cost;

 bool critical_hit = scored_crit(mondodge);

 int bash_dam = roll_bash_damage(critter, critical_hit);
 int cut_dam  = roll_cut_damage(critter, critical_hit);
 int stab_dam = roll_stab_damage(critter, critical_hit);

 int pain = 0; // Boost to pain; required for perform_technique

// Pick one or more special attacks
 matec_id tec_id = pick_technique(g, critter, NULL, critical_hit, allow_grab);
 ma_technique technique = ma_techniques[tec_id];


// Handles effects as well; not done in melee_affect_*
 if (tec_id != "tec_none")
  perform_technique(technique, g, critter, NULL, bash_dam, cut_dam, stab_dam, pain);

 if (weapon.has_flag("FLAMING")) {
   critter->add_effect(ME_ONFIRE, rng(3, 4));
 }
 critter->speed -= int(pain / 2);

// Mutation-based attacks
 perform_special_attacks(g, critter, NULL, bash_dam, cut_dam, stab_dam);

    message = melee_message(technique.id, *this, bash_dam, cut_dam, stab_dam);

// Make a rather quiet sound, to alert any nearby monsters
 if (is_quiet()) // check martial arts silence
  g->sound(posx, posy, 2, "");
 else
  g->sound(posx, posy, 8, "");

 // Handles speed penalties to monster & us, etc
 std::string specialmsg = melee_special_effects(g, critter, this, critical_hit, bash_dam, cut_dam, stab_dam);

 int dam = bash_dam + (cut_dam > stab_dam ? cut_dam : stab_dam);

  if (g->u_see(critter)) {
      player_hit_message(g, this, message, target_name, dam, critical_hit);
  }

  if (!specialmsg.empty())
    g->add_msg(specialmsg.c_str());

 bool bashing = (bash_dam >= 10 && !unarmed_attack());
 bool cutting = (cut_dam >= 10);
 bool stabbing = (stab_dam >= 5);
 melee_practice(g->turn, *this, true, unarmed_attack(), bashing, cutting, stabbing);

 if (allow_grab && technique.grabs) {
// Move our weapon to a temp slot, if it's not unarmed
  if (!unarmed_attack()) {
   item tmpweap = remove_weapon();
   dam += hit_mon(g, critter, false); // False means a second grab isn't allowed
   weapon = tmpweap;
  } else
   dam += hit_mon(g, critter, false); // False means a second grab isn't allowed
 }

 if (dam >= 5 && has_artifact_with(AEP_SAP_LIFE))
  healall( rng(dam / 10, dam / 5) );

  ma_onattack_effects(); // trigger martial arts on-attack effects
 return dam;
}

void player::hit_player(game *g, player &p, bool allow_grab)
{
  bool is_u = (this == &(g->u)); // Affects how we'll display messages

  if (is_u && p.is_npc()) {
    npc* npcPtr = dynamic_cast<npc*>(&p);
    npcPtr->make_angry();
  }

  std::string message = is_u ? _("You hit %s") : _("<npcname> hits %s");

// Divide their dodge roll by 2 if this is a grab
  int target_dodge = (allow_grab ? p.dodge_roll(g) / 2 : p.dodge_roll(g));
  int hit_value = hit_roll() - target_dodge;
  bool missed = (hit_roll() <= 0);

  int move_cost = attack_speed(*this);

  if (missed) {
    // trigger on-dodge effects
    p.ma_ondodge_effects();

    int stumble_pen = stumble(*this);
    if (is_u) { // Only display messages if this is the player
      if (has_miss_recovery_tec())
        g->add_msg(_("You feint."));
      else if (stumble_pen >= 60)
        g->add_msg(_("You miss and stumble with the momentum."));
      else if (stumble_pen >= 10)
        g->add_msg(_("You swing wildly and miss."));
      else
        g->add_msg(_("You miss."));
    }
    melee_practice(g->turn, *this, false, unarmed_attack(),
                  weapon.is_bashing_weapon(), weapon.is_cutting_weapon(),
                  (weapon.has_flag("SPEAR") || weapon.has_flag("STAB")));
    move_cost += stumble_pen;
    if (has_miss_recovery_tec())
      move_cost = rng(move_cost / 3, move_cost);
    moves -= move_cost;
    return;
  }

 moves -= move_cost;

 if (p.uncanny_dodge(is_u)) { return; }

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

    std::string bodypart = body_part_name(bp_hit, side);
    std::string target_name;
    if (p.is_npc()) {
        //~ someone's bodypart, e.g. "Kate's leg"
        target_name = string_format(_("%1$s's %2$s"),
                                    p.name.c_str(), bodypart.c_str());
    } else {
        //~ your bodypart, e.g. "your head"
        target_name = string_format(_("your %s"), bodypart.c_str());
    }

 bool critical_hit = scored_crit(target_dodge);

 int bash_dam = roll_bash_damage(NULL, critical_hit);
 int cut_dam  = roll_cut_damage(NULL, critical_hit);
 int stab_dam = roll_stab_damage(NULL, critical_hit);

 p.block_hit(g, bp_hit, side, bash_dam, cut_dam, stab_dam);

 if (bash_dam + cut_dam + stab_dam <= 0)
  return; // Defensive technique canceled our attack!

 if (critical_hit) // Crits cancel out Toad Style's armor boost
  p.rem_disease("armor_boost");

 int pain = 0; // Boost to pain; required for perform_technique

// Pick one or more special attacks
 matec_id tec_id = pick_technique(g, NULL, &p, critical_hit, allow_grab);
 ma_technique technique = ma_techniques[tec_id];

// Handles effects as well; not done in melee_affect_*
 if (tec_id != "tec_none")
  perform_technique(technique, g, NULL, &p, bash_dam, cut_dam, stab_dam, pain);

 if (weapon.has_flag("FLAMING")) {
   p.add_disease("onfire", rng(2, 3));
 }
 p.pain += pain;

// Mutation-based attacks
 perform_special_attacks(g, NULL, &p, bash_dam, cut_dam, stab_dam);

// Handles speed penalties to monster & us, etc
 melee_special_effects(g, NULL, &p, critical_hit, bash_dam, cut_dam, stab_dam);

// Make a rather quiet sound, to alert any nearby monsters
 if (!is_quiet()) // check martial arts silence
  g->sound(posx, posy, 8, "");

 p.hit(g, bp_hit, side, bash_dam, (cut_dam > stab_dam ? cut_dam : stab_dam));

    message = melee_message(technique.id, *this, bash_dam, cut_dam, stab_dam);
    int dam = bash_dam + (cut_dam > stab_dam ? cut_dam : stab_dam);
    player_hit_message(g, this, message, target_name, dam, critical_hit);

 bool bashing = (bash_dam >= 10 && !unarmed_attack());
 bool cutting = (cut_dam >= 10 && cut_dam >= stab_dam);
 bool stabbing = (stab_dam >= 10 && stab_dam >= cut_dam);
 melee_practice(g->turn, *this, true, unarmed_attack(), bashing, cutting, stabbing);

 if (dam >= 5 && has_artifact_with(AEP_SAP_LIFE))
  healall( rng(dam / 10, dam / 5) );

 if (allow_grab && technique.grabs) {
// Move our weapon to a temp slot, if it's not unarmed
  if (p.has_grab_break_tec() &&
      dice(p.dex_cur + p.skillLevel("melee"), 12) >
      dice(dex_cur + skillLevel("melee"), 10)) {
   g->add_msg_player_or_npc(&p, _("You break the grab!"),
                                _("<npcname> breaks the grab!"));
  } else if (!unarmed_attack()) {
   item tmpweap = remove_weapon();
   hit_player(g, p, false); // False means a second grab isn't allowed
   weapon = tmpweap;
  } else
   hit_player(g, p, false); // False means a second grab isn't allowed
 }

  ma_onattack_effects(); // trigger martial arts on-attack effects
 /*
 if (tech_def.counters) {
  g->add_msg_if_player(&p, _("Counter-attack!"));
  p.hit_player(g, *this);
 }
 */
}

int stumble(player &u)
{
 int stumble_pen = 2 * u.weapon.volume() + (u.weapon.weight() / 113);
 if (u.has_trait("DEFT"))
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
 int num_crits = 0;

// Weapon to-hit roll
 int chance = 25;
 if (unarmed_attack()) { // Unarmed attack: 1/2 of unarmed skill is to-hit
  for (int i = 1; i <= int(skillLevel("unarmed") * .5); i++)
   chance += (50 / (2 + i));
 }
 if (weapon.type->m_to_hit > 0) {
  for (int i = 1; i <= weapon.type->m_to_hit; i++)
   chance += (50 / (2 + i));
 } else if (weapon.type->m_to_hit < 0) {
  for (int i = 0; i > weapon.type->m_to_hit; i--)
   chance /= 2;
 }
 if (rng(0, 99) < chance + 4 * disease_intensity("attack_boost"))
  num_crits++;

// Dexterity to-hit roll
// ... except sometimes we don't use dexteiry!
 int stat = dex_cur;

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

 if (weapon.is_bashing_weapon() && skillLevel("bashing") > best_skill)
  best_skill = skillLevel("bashing");
 if (weapon.is_cutting_weapon() && skillLevel("cutting") > best_skill)
  best_skill = skillLevel("cutting");
 if ((weapon.has_flag("SPEAR") || weapon.has_flag("STAB")) &&
     skillLevel("stabbing") > best_skill)
  best_skill = skillLevel("stabbing");
 if (unarmed_attack() && skillLevel("unarmed") > best_skill)
  best_skill = skillLevel("unarmed");

 best_skill += int(skillLevel("melee") / 2.5);

 chance = 25;
 if (best_skill > 3) {
  for (int i = 3; i < best_skill; i++)
   chance += (50 / (2 + i));
 } else if (best_skill < 3) {
  for (int i = 3; i > best_skill; i--)
   chance /= 2;
 }
 if (rng(0, 99) < chance + 4 * disease_intensity("attack_boost"))
  num_crits++;

 if (num_crits == 3)
  return true;
 else if (num_crits == 2)
  return (hit_roll() >= target_dodge * 1.5 && !one_in(4));

 return false;
}

int player::dodge(game *g)
//Returns 1/2*DEX + dodge skill level + static bonuses from mutations
//Return numbers range from around 4 (starting player, no boosts) to 29 (20 DEX, 10 dodge, +9 mutations)
{
    //If we're asleep or busy we can't dodge
    if (has_disease("sleep") || has_disease("lying_down")) {return 0;}
    if (activity.type != ACT_NULL) {return 0;}

    int ret = (dex_cur / 2);
    ret += skillLevel("dodge");
    ret += disease_intensity("dodge_boost");
    ret -= (encumb(bp_legs) / 2) + encumb(bp_torso);
    ret += int(current_speed(g) / 150); //Faster = small dodge advantage

    // add martial arts bonus
    ret += mabuff_dodge_bonus();

    //Mutations
    if (has_trait("TAIL_LONG")) {ret += 2;}
    if (has_trait("TAIL_CATTLE")) {ret+= 1;}
    if (has_trait("TAIL_RAT")) {ret+= 2;}
    if (has_trait("TAIL_THICK")) {ret+= 1;}
    if (has_trait("TAIL_RAPTOR")) {ret+= 3;}
    if (has_trait("TAIL_FLUFFY")) {ret += 4;}
    if (has_trait("WHISKERS")) {ret += 1;}
    if (has_trait("WINGS_BAT")) {ret -= 3;}

    if (str_max >= 16) {ret--;} // Penalty if we're huge
    else if (str_max <= 5) {ret++;} // Bonus if we're small

    if (dodges_left <= 0) // We already dodged this turn
    {
        if (rng(0, skillLevel("dodge") + dex_cur + 15) <= skillLevel("dodge") + dex_cur)
        {
            ret = rng(ret/2, ret); //Penalize multiple dodges per turn
        }
        else
        {
            ret = 0;
        }
    }
    dodges_left--;
    return ret;
}

int player::dodge_roll(game *g)
{
    return dice(dodge(g), 10); //Matches NPC and monster dodge_roll functions
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

int player::roll_bash_damage(monster *critter, bool crit)
{
 int ret = 0;
 int stat = str_cur; // Which stat determines damage?
 int skill = skillLevel("bashing"); // Which skill determines damage?

 stat += mabuff_bash_bonus();

 if (unarmed_attack())
  skill = skillLevel("unarmed");

 if(weapon.typeId() =="style_crane"){
   stat = (dex_cur * 2 + str_cur) / 3;
 } else if(weapon.typeId() == "style_snake"){
   stat = int(str_cur + per_cur) / 2;
 } else if(weapon.typeId() == "style_dragon"){
   stat = int(str_cur + int_cur) / 2;
 }

 ret = base_damage(true, stat);

// Drunken Master damage bonuses
 if (has_trait("DRUNKEN") && has_disease("drunk")) {
// Remember, a single drink gives 600 levels of "drunk"
  int mindrunk, maxdrunk;
  if (unarmed_attack()) {
   mindrunk = disease_duration("drunk") / 600;
   maxdrunk = disease_duration("drunk") / 250;
  } else {
   mindrunk = disease_duration("drunk") / 900;
   maxdrunk = disease_duration("drunk") / 400;
  }
  ret += rng(mindrunk, maxdrunk);
 }

 int bash_dam = int(stat / 2) + weapon.damage_bash(),
     bash_cap = 5 + stat + skill;

 if (unarmed_attack())
  bash_dam = rng(0, int(stat / 2) + skillLevel("unarmed"));

 if (crit) {
  bash_dam *= 1.5;
  bash_cap *= 2;
 }

 if (bash_dam > bash_cap)// Cap for weak characters
  bash_dam = (bash_cap * 3 + bash_dam) / 4;

 if (critter != NULL && critter->has_flag(MF_PLASTIC))
  bash_dam /= rng(2, 4);

 int bash_min = bash_dam / 4;

 bash_dam = rng(bash_min, bash_dam);

 if (bash_dam < skill + int(stat / 2))
  bash_dam = rng(bash_dam, skill + int(stat / 2));

 ret += bash_dam;

 ret += disease_intensity("damage_boost");

// Finally, extra crit effects
 if (crit) {
  ret += int(stat / 2);
  ret += skill;
  if (critter != NULL)
   ret -= critter->armor_bash() / 2;
 } else if (critter != NULL)
  ret -= critter->armor_bash();

 return (ret < 0 ? 0 : ret);
}

int player::roll_cut_damage(monster *critter, bool crit)
{
 if (weapon.has_flag("SPEAR"))
  return 0;  // Stabs, doesn't cut!
 int z_armor_cut = (critter == NULL ? 0 : critter->armor_cut() - skillLevel("cutting") / 2);

 if (crit)
  z_armor_cut /= 2;
 if (z_armor_cut < 0)
  z_armor_cut = 0;

 double ret = mabuff_cut_bonus() + weapon.damage_cut() - z_armor_cut;

 if (unarmed_attack() && !wearing_something_on(bp_hands)) {
  if (has_trait("CLAWS") || has_trait("CLAWS_RETRACT"))
   ret += 6;
  if (has_bionic("bio_razors"))
   ret += 4;
  if (has_trait("TALONS"))
   ret += 6 + ((int)skillLevel("unarmed") > 8 ? 8 : (int)skillLevel("unarmed"));
  if (has_trait("SLIME_HANDS") && (critter == NULL || !critter->has_flag(MF_ACIDPROOF)))
   ret += rng(4, 6);
 }
  
 if (ret <= 0)
  return 0; // No negative damage!

// 80%, 88%, 96%, 104%, 112%, 116%, 120%, 124%, 128%, 132%
 if (skillLevel("cutting") <= 5)
  ret *= 0.8 + 0.08 * skillLevel("cutting");
 else
  ret *= 0.92 + 0.04 * skillLevel("cutting");

 if (crit)
  ret *= 1.0 + (skillLevel("cutting") / 12.0);

 return ret;
}

int player::roll_stab_damage(monster *critter, bool crit)
{
 double ret = 0;
 int z_armor = (critter == NULL ? 0 : critter->armor_cut() - 3 * skillLevel("stabbing"));

 if (crit)
  z_armor /= 3;
 if (z_armor < 0)
  z_armor = 0;

 if (unarmed_attack() && !wearing_something_on(bp_hands)) {
  ret = 0 - z_armor;
  if (has_trait("CLAWS") || has_trait("CLAWS_RETRACT"))
   ret += 6;
  if (has_trait("NAILS") && z_armor == 0)
   ret++;
  if (has_bionic("bio_razors"))
   ret += 4;
  if (has_trait("THORNS"))
   ret += 4;
 } else if (weapon.has_flag("SPEAR") || weapon.has_flag("STAB"))
  ret = int((weapon.damage_cut() - z_armor));
 else
  return 0; // Can't stab at all!

 if (critter != NULL && critter->speed > 100) { // Bonus against fast monsters
  int speed_min = (critter->speed - 100) / 10, speed_max = (critter->speed - 100) / 5;
  int speed_dam = rng(speed_min, speed_max);
  if (speed_dam > ret * 2)
   speed_dam = ret * 2;
  if (speed_dam > 0)
   ret += speed_dam;
 }

 if (ret <= 0)
  return 0; // No negative stabbing!

 if (crit) {
  double multiplier = 1.0 + (skillLevel("stabbing") / 5.0);
  if (multiplier > 2.5)
   multiplier = 2.5;
  ret *= multiplier;
 }

 return ret;
}

// Chance of a weapon sticking is based on weapon attack type.
// Only an issue for cutting and piercing weapons.
// Attack modes are "CHOP", "STAB", and "SLICE".
// "SPEAR" is synonymous with "STAB".
// Weapons can have a "low_stick" flag indicating they
// Have a feature to prevent sticking, such as a spear with a crossbar,
// Or a stabbing blade designed to resist sticking.
int player::roll_stuck_penalty(bool stabbing)
{
    // The cost of the weapon getting stuck, in units of move points.
    const int weapon_speed = attack_speed(*this);
    int stuck_cost = weapon_speed;
    const int attack_skill = stabbing ? skillLevel("stabbing") : skillLevel("cutting");
    const float cut_damage = weapon.damage_cut();
    const float bash_damage = weapon.damage_bash();
    float cut_bash_ratio = 0.0;

    // Scale cost along with the ratio between cutting and bashing damage of the weapon.
    if( cut_damage > 0.0 || bash_damage > 0.0 )
    {
        cut_bash_ratio = cut_damage / ( cut_damage + bash_damage );
    }
    stuck_cost *= cut_bash_ratio;

    if( weapon.has_flag("SLICE") )
    {
        // Slicing weapons assumed to have a very low chance of sticking.
        stuck_cost *= 0.25;
    }
    else if( weapon.has_flag("STAB") || weapon.has_flag("SPEAR") )
    {
        // Stabbing has a moderate change of sticking.
        stuck_cost *= 0.50;
    }
    else if( weapon.has_flag("CHOP") )
    {
        // Chopping has a high chance of sticking.
        stuck_cost *= 1.00;
    }
    else
    {
        // Items with no attack type are assumed to be improvised weapons,
        // and get a very high stick cost.
        stuck_cost *= 2.00;
    }

    if( weapon.has_flag("NON_STUCK") )
    {
        // Greatly reduce sticking frequency/severity if the weapon has an anti-sticking feature.
        stuck_cost /= 4;
    }

    // Reduce cost based on player skill, by 10.5 move/level on average.
    stuck_cost -= dice( attack_skill, 20 );

    // Make sure cost doesn't go negative.
    stuck_cost = std::max( stuck_cost, 0 );
    // Cap stuck penalty at 2x weapon speed.
    stuck_cost = std::min( stuck_cost, 2*weapon_speed );

    return stuck_cost;
}

matec_id player::pick_technique(game *g, monster *critter, player *p,
                                    bool crit, bool allowgrab)
{
    (void)allowgrab; //FIXME: is this supposed to be being used for something?

 if (critter == NULL && p == NULL)
  return "tec_none";

 std::vector<matec_id> all = get_all_techniques();

 std::vector<matec_id> possible;
 bool downed = ((critter && !critter->has_effect(ME_DOWNED)) ||
                (p && !p->has_disease("downed"))  );

  // first add non-aoe tecs
  for (std::vector<matec_id>::const_iterator it = all.begin();
      it != all.end(); ++it) {
    ma_technique tec = ma_techniques[*it];

    // skip defensive techniques
    if (tec.defensive) continue;

    // if crit then select only from crit tecs
    if ((crit && !tec.crit_tec) || (!crit && tec.crit_tec)) continue;

    // don't apply downing techniques to someone who's already downed
    if (downed && tec.down_dur > 0) continue;

    // don't apply disarming techniques to someone without a weapon
    //TODO: these are the stat reqs for tec_disarm
    // dice(   dex_cur +    skillLevel("unarmed"),  8) >
    // dice(p->dex_cur + p->skillLevel("melee"),   10))
    if (tec.disarms && !(!critter && p->weapon.typeId() != "null")) continue;

    // ignore aoe tecs for a bit
    if (tec.aoe.length() > 0) continue;

    if (tec.is_valid_player(*this))
      possible.push_back(tec.id);
  }

  // now add aoe tecs (since they depend on if we have other tecs or not)
  for (std::vector<matec_id>::const_iterator it = all.begin();
      it != all.end(); ++it) {
    ma_technique tec = ma_techniques[*it];

    // don't use aoe tecs if there's only one target
    if (tec.aoe.length() > 0) {
      int enemy_count = 0;
      for (int x = posx - 1; x <= posx + 1; x++) {
        for (int y = posy - 1; y <= posy + 1; y++) {
          int mondex = g->mon_at(x, y);
          if (mondex != -1) {
            if (g->zombie(mondex).friendly == 0)
            enemy_count++;
            else
            enemy_count -= 2;
          }
          int npcdex = g->npc_at(x, y);
          if (npcdex != -1) {
            if (g->active_npc[npcdex]->attitude == NPCATT_KILL)
            enemy_count++;
            else
            enemy_count -= 2;
          }
        }
      }
      if (tec.is_valid_player(*this) &&
          enemy_count >= (possible.empty() ? 1 : 2)) {
        possible.push_back(tec.id);
      }
    }
  }

  if (possible.empty()) return "tec_none";

  return possible[ rng(0, possible.size() - 1) ];
}

bool player::has_technique(matec_id id) {
  return weapon.has_technique(id) ||
    martialarts[style_selected].has_technique(*this, id);
}

void player::perform_technique(ma_technique technique, game *g, monster *critter,
                               player *p, int &bash_dam, int &cut_dam,
                               int &stab_dam, int &pain)
{

    const bool mon = (critter != NULL);
    const bool npc = (p != NULL && p->is_npc());
    const bool you = (p == &(g->u));

    std::string target;
    if (mon) {
        target = string_format(_("the %s"), critter->name().c_str());
    } else if (npc) {
        target = p->name;
    } else {
        // "you" handled separately
        //TODO alert the user if this happens
        //This is done to avoid NULL derefrences
        debugmsg("no valid target in perform_technique");
        return;
    }

  bash_dam += technique.bash;
  cut_dam += technique.cut;
  stab_dam += technique.cut; // cut affects stab damage too since only one of cut/stab is used

  bash_dam *= technique.bash_mult;
  cut_dam *= technique.cut_mult;
  stab_dam *= technique.cut_mult;

  int tarx = (mon ? critter->posx() : p->posx), tary = (mon ? critter->posy() : p->posy);

  if (technique.quick) {
    moves += int(attack_speed(*this) / 2);
  }
// The rest affect our target, and thus depend on critter vs. p

  if (technique.down_dur > 0) {
    if (critter != NULL && !critter->has_flag(MF_FLIES)) {
      critter->add_effect(ME_DOWNED, rng(1, 2));
      bash_dam += critter->fall_damage();
    } else if (p != NULL && p->weapon.typeId() != "style_judo" &&
      !p->is_throw_immune()) {
      p->add_disease("downed", rng(1, 2));
      bash_dam += 3;
    }
  }

  if (technique.stun_dur > 0) {
    if (critter != NULL)
      critter->add_effect(ME_STUNNED, rng(1, technique.stun_dur));
    else if (p != NULL)
      p->add_disease("stunned", rng(1, technique.stun_dur/2));
  }

  if (technique.knockback_dist > 0) {
    int kb_offset = rng(
      -technique.knockback_spread,
      technique.knockback_spread
    );
    if (critter != NULL) {
      critter->knock_back_from(g, posx+kb_offset, posy+kb_offset);
    } else if (p != NULL) {
      p->knock_back_from(g, posy+kb_offset, posy+kb_offset);
    }
  }

  if (technique.pain > 0) {
    pain += rng(technique.pain/2, technique.pain);
  }

    if (technique.disarms) {
        g->m.add_item_or_charges(p->posx, p->posy, p->remove_weapon());
        if (you) {
            g->add_msg_if_npc(this, _("<npcname> disarms you!"));
        } else {
            g->add_msg_player_or_npc(this, _("You disarm %s!"),
                                     _("<npcname> disarms %s!"),
                                     target.c_str() );
        }
    }

  if (technique.aoe.length() > 0) {
    int count_hit = 0;
    for (int x = posx - 1; x <= posx + 1; x++) {
      for (int y = posy - 1; y <= posy + 1; y++) {
        if (x != tarx || y != tary) { // Don't double-hit our target
          int mondex = g->mon_at(x, y);
          if (mondex != -1 && hit_roll() >= rng(0, 5) + g->zombie(mondex).dodge_roll()) {
            count_hit++;
            int dam = roll_bash_damage(&(g->zombie(mondex)), false) +
                roll_cut_damage (&(g->zombie(mondex)), false);
            if (g->zombie(mondex).hurt(dam)) {
                g->zombie(mondex).die(g);
            }
            if (weapon.has_flag("FLAMING"))  { // Add to wide attacks
                g->zombie(mondex).add_effect(ME_ONFIRE, rng(3, 4));
            }
            std::string temp_target = string_format(_("the %s"), g->zombie(mondex).name().c_str());
            g->add_msg_player_or_npc( this, _("You hit %s!"), _("<npcname> hits %s!"), temp_target.c_str() );
          }
          int npcdex = g->npc_at(x, y);
          if (npcdex != -1 &&
              hit_roll() >= rng(0, 5) + g->active_npc[npcdex]->dodge_roll(g)) {
            count_hit++;
            int dam = roll_bash_damage(NULL, false);
            int cut = roll_cut_damage (NULL, false);
            g->active_npc[npcdex]->hit(g, bp_legs, 3, dam, cut);
            if (weapon.has_flag("FLAMING")) {// Add to wide attacks
                g->active_npc[npcdex]->add_disease("onfire", rng(2, 3));
            }
            g->add_msg_player_or_npc( this, _("You hit %s!"), _("<npcname> hits %s!"), g->active_npc[npcdex]->name.c_str() );

            g->active_npc[npcdex]->add_disease("onfire", rng(2, 3));
          }
        }
      }
    }
    g->add_msg_if_player(p, ngettext("%d enemy hit!", "%d enemies hit!", count_hit), count_hit);

  }

}

// this would be i2amroy's fix, but it's kinda handy
bool player::can_weapon_block()
{
    return (weapon.has_technique("WBLOCK_1") ||
            weapon.has_technique("WBLOCK_2") ||
            weapon.has_technique("WBLOCK_3"));
}

bool player::block_hit(game *g, body_part &bp_hit, int &side,
                       int &bash_dam, int &cut_dam, int &stab_dam) {

  ma_ongethit_effects(); // fire martial arts on-getting-hit-triggered effects
  // these fire even if the attack is blocked (you still got hit)

  if (blocks_left < 1)
      return false;

  if (unarmed_attack() && can_block()) {
    //Choose which body part to block with
    if (can_leg_block() && can_arm_block())
      bp_hit = one_in(2) ? bp_legs : bp_arms;
    else if (can_leg_block())
      bp_hit = bp_legs;
    else
      bp_hit = bp_arms;

    // Choose what side to block with.
    if (bp_hit == bp_legs)
      side = hp_cur[hp_leg_r] > hp_cur[hp_leg_l];
    else
      side = hp_cur[hp_arm_r] > hp_cur[hp_arm_l];

    g->add_msg_player_or_npc( this, _("You block with your %s!"),
    _("<npcname> blocks with their %s!"),
    body_part_name(bp_hit, side).c_str());

    bash_dam *= .5;

    bash_dam -= mabuff_block_bonus();
    bash_dam = bash_dam < 0 ? 0 : bash_dam;
  }
  else if (can_arm_block() || can_weapon_block()) {
    float mult = 1.0f;
    // If we are using a weapon, apply extra reductions
    if (weapon.has_technique("WBLOCK_1")) {
      mult = 0.4;
    } else if (weapon.has_technique("WBLOCK_2")) {
      mult = 0.15;
    } else if (weapon.has_technique("WBLOCK_3")) {
      mult = 0.05;
    } else {
      mult = 0.5; // always at least as good as unarmed
    }
    g->add_msg_player_or_npc( this, _("You block with your %s!"),
                _("<npcname> blocks with their %s!"), weapon.tname().c_str() );
    bash_dam *= mult;
    cut_dam *= mult;
    stab_dam *= mult;
    // then convert cut/stab into bash
    bash_dam += cut_dam + stab_dam;
    cut_dam = stab_dam = 0;

    bash_dam -= mabuff_block_bonus();
    bash_dam = bash_dam < 0 ? 0 : bash_dam;
  }
  blocks_left--;

  ma_onblock_effects(); // fire martial arts block-triggered effects

  return true;
}

void player::perform_special_attacks(game *g, monster *critter, player *p,
                                     int &bash_dam, int &cut_dam, int &stab_dam)
{
 bool can_poison = false;
 int bash_armor = (critter == NULL ? 0 : critter->armor_bash());
 int cut_armor  = (critter == NULL ? 0 : critter->armor_cut());
 std::vector<special_attack> special_attacks = mutation_attacks(critter, p);

    std::string target;
    if (critter != NULL) {
        target = string_format(_("the %s"), critter->name().c_str());
    } else if (p != NULL) {
        target = p->name.c_str();
    } else {
        target = "the bug";
    }

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

 if (can_poison && has_trait("POISONOUS")) {
  if (critter != NULL) {
   if (!critter->has_effect(ME_POISONED))
    g->add_msg_if_player(p,_("You poison %s!"), target.c_str());
   critter->add_effect(ME_POISONED, 6);
  } else if (p != NULL) {
   if (!p->has_disease("poison"))
    g->add_msg_if_player(p,_("You poison %s!"), target.c_str());
   p->add_disease("poison", 6);
  }
 }
}

std::string player::melee_special_effects(game *g, monster *critter, player *p, bool crit,
                                   int &bash_dam, int &cut_dam, int &stab_dam)
{
    std::stringstream dump;

    if (critter == NULL && p == NULL) {
        return NULL;
    }
    const bool mon = (critter != NULL);
    const bool npc = (p != NULL && p->is_npc());
    const bool you = (p == &(g->u));

    std::string target;
    if (mon) {
        target = string_format(_("the %s"), critter->name().c_str());
    } else if (npc) {
        target = p->name;
    } else {
        target = "a bug";
        // "you" handled separately
    }

 int tarposx = (mon ? critter->posx() : p->posx), tarposy = (mon ? critter->posy() : p->posy);

// Bashing effecs
 if (mon)
  critter->moves -= rng(0, bash_dam * 2);
 else
  p->moves -= rng(0, bash_dam * 2);

// Bashing crit
 if (crit && !unarmed_attack()) {
  int turns_stunned = int(bash_dam / 20) + rng(0, int(skillLevel("bashing") / 2));
  if (turns_stunned > 6)
   turns_stunned = 6;
  if (turns_stunned > 0) {
   if (mon)
    critter->add_effect(ME_STUNNED, turns_stunned);
   else
    p->add_disease("stunned", 1 + turns_stunned / 2);
  }
 }

    // Stabbing effects
    int stab_moves = rng(stab_dam / 2, stab_dam * 1.5);
    if (crit) {
        stab_moves *= 1.5;
    }
    if (stab_moves >= 150) {
        if (you) {
            // can the player force their self to the ground? probably not.
            g->add_msg_if_npc(this, _("<npcname> forces you to the ground!"));
        } else {
            g->add_msg_player_or_npc(this, _("You force %s to the ground!"),
                                     _("<npcname> forces %s to the ground!"),
                                     target.c_str() );
        }
        if (mon) {
            critter->add_effect(ME_DOWNED, 1);
            critter->moves -= stab_moves / 2;
        } else {
            p->add_disease("downed", 1);
            p->moves -= stab_moves / 2;
        }
    } else if (mon) {
        critter->moves -= stab_moves;
    } else {
        p->moves -= stab_moves;
    }

// Bonus attacks!
 bool shock_them = (has_active_bionic("bio_shock") && power_level >= 2 &&
                    (unarmed_attack() || weapon.made_of("iron") ||
                     weapon.made_of("steel") || weapon.made_of("silver") ||
                     weapon.made_of("gold")) &&
                    (!mon || !critter->has_flag(MF_ELECTRIC)) && one_in(3));

 bool drain_them = (has_active_bionic("bio_heat_absorb") && power_level >= 1 &&
                    !is_armed() && (!mon || critter->has_flag(MF_WARM)));

 drain_them &= one_in(2); // Only works half the time

    if (shock_them) {
        power_level -= 2;
        int shock = rng(2, 5);
        if (mon) {
            critter->hurt( shock * rng(1, 3) );
            critter->moves -= shock * 180;
            g->add_msg_player_or_npc(this, _("You shock %s."),
                                     _("<npcname> shocks %s."),
                                     target.c_str());
        } else {
            p->hurt(g, bp_torso, -1, shock * rng(1, 3));
            p->moves -= shock * 80;
        }
    }

    if (drain_them) {
        power_level--;
        charge_power(rng(0, 2));
        if (you) {
            g->add_msg_if_npc(this, _("<npcname> drains your body heat!"));
        } else {
            g->add_msg_player_or_npc(this, _("You drain %s's body heat!"),
                                     _("<npcname> drains %s's body heat!"),
                                     target.c_str());
        }
        if (mon) {
            critter->moves -= rng(80, 120);
            critter->speed -= rng(4, 6);
        } else {
            p->moves -= rng(80, 120);
        }
    }

 bool conductive = !wearing_something_on(bp_hands) && weapon.conductive();

 if (mon && critter->has_flag(MF_ELECTRIC) && conductive) {
  hurtall(rng(0, 1));
  moves -= rng(0, 50);
  g->add_msg_if_player(p, _("Contact with %s shocks you!"), target.c_str());
 }

 //Hurting the wielder from poorly-chosen weapons
 if(weapon.has_flag("HURT_WHEN_WIELDED") && x_in_y(2, 3)) {
     g->add_msg_if_player(p, _("The %s cuts your hand!"), weapon.tname().c_str());
     p->hit(g, bp_hands, 0, 0, weapon.damage_cut());
     if (weapon.is_two_handed(this)) { // Hurt left hand too, if it was big
       hit(g, bp_hands, 1, 0, weapon.damage_cut());
     }
 }

// Glass weapons shatter sometimes
 if (weapon.made_of("glass") &&
     rng(0, weapon.volume() + 8) < weapon.volume() + str_cur) {
     g->add_msg_player_or_npc( p, _("Your %s shatters!"), _("<npcname>'s %s shatters!"),
                               weapon.tname().c_str() );

  g->sound(posx, posy, 16, "");
// Dump its contents on the ground
  for (int i = 0; i < weapon.contents.size(); i++)
   g->m.add_item_or_charges(posx, posy, weapon.contents[i]);
  hit(g, bp_arms, 1, 0, rng(0, weapon.volume() * 2));// Take damage
  if (weapon.is_two_handed(this))// Hurt left arm too, if it was big
   hit(g, bp_arms, 0, 0, rng(0, weapon.volume()));
  cut_dam += rng(0, 5 + int(weapon.volume() * 1.5));// Hurt the monster extra
  remove_weapon();
 }

// Getting your weapon stuck
 int cutting_penalty = roll_stuck_penalty(stab_dam > cut_dam);
 if (weapon.has_flag("MESSY")) { // e.g. chainsaws
  cutting_penalty /= 6; // Harder to get stuck
  for (int x = tarposx - 1; x <= tarposx + 1; x++) {
   for (int y = tarposy - 1; y <= tarposy + 1; y++) {
    if (!one_in(3)) {
      g->m.add_field(g, x, y, fd_blood, 1);
    }
   }
  }
 }
 if (!unarmed_attack() && cutting_penalty > dice(str_cur * 2, 20) &&
     !critter->is_hallucination()) {
     if (you) {
         dump << string_format(_("Your %s gets stuck in %s, pulling it our of your hands!"),
                               weapon.tname().c_str(), target.c_str());
     }
     if(weapon.has_flag("HURT_WHEN_PULLED") && one_in(3)) {
         //Sharp objects that injure wielder when pulled from hands (so cutting damage only)
         dump << std::endl << string_format(_("You are hurt by the %s being pulled from your hands!"),
                                            weapon.tname().c_str());
         p->hit(g, bp_hands, random_side(bp_hands), 0, (weapon.damage_cut() / 2));
     }
     if (mon) {
         if (weapon.has_flag("SPEAR") || weapon.has_flag("STAB")) {
             critter->speed *= .7;
         } else {
             critter->speed *= .85;
         }
         critter->add_item(remove_weapon());
         g->m.add_item_or_charges(posx, posy, remove_weapon());
     }
 } else {
  if (mon && (cut_dam >= critter->hp || stab_dam >= critter->hp)) {
   cutting_penalty /= 2;
   cutting_penalty -= rng(skillLevel("cutting"), skillLevel("cutting") * 2 + 2);
  }
  if (cutting_penalty > 0)
   moves -= cutting_penalty;
  if (cutting_penalty >= 50 && !critter->is_hallucination()) {
    if (you)
    dump << string_format(_("Your %s gets stuck in %s but you yank it free!"), weapon.tname().c_str(), target.c_str());
  }
  if (mon && (weapon.has_flag("SPEAR") || weapon.has_flag("STAB")))
   critter->speed *= .9;
 }

// Finally, some special effects for martial arts
  // multiply damage by style damage_mults
  bash_dam *= mabuff_bash_mult();
  cut_dam *= mabuff_cut_mult();
  stab_dam *= mabuff_cut_mult();

  // on-hit effects for martial arts
  ma_onhit_effects();

  return dump.str();
}

std::vector<special_attack> player::mutation_attacks(monster *critter, player *p)
{
    std::vector<special_attack> ret;

    if (critter == NULL && p == NULL) {
        return ret;
    }

    const bool mon = (critter != NULL);
    const bool npc = (p != NULL && p->is_npc());
    const bool male = (p != NULL && p->male);
    const bool is_u = !is_npc();

    std::string target;
    if (mon) {
        target = string_format(_("the %s"), critter->name().c_str());
    } else if (npc) {
        target = p->name;
    } else {
        target = "a bug";
        // "you" handled separately
    }

 //Having lupine or croc jaws makes it much easier to sink your fangs into people; Ursine/Feline, not so much
    if (has_trait("FANGS") && (
            (!wearing_something_on(bp_mouth) && !has_trait("MUZZLE") && !has_trait("LONG_MUZZLE") &&
            one_in(20 - dex_cur - skillLevel("unarmed"))) ||
            (has_trait("MUZZLE") && one_in(18 - dex_cur - skillLevel("unarmed"))) ||
            (has_trait("LONG_MUZZLE") && one_in(15 - dex_cur - skillLevel("unarmed"))) ||
            (has_trait("BEAR_MUZZLE") && one_in(20 - dex_cur - skillLevel("unarmed"))))) {
        special_attack tmp;
        tmp.stab = 20;
        if (is_u) {
            tmp.text = string_format(_("You sink your fangs into %s!"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%s sinks his fangs into %s!"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%s sinks her fangs into %s!"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    if (!has_trait("FANGS") && has_trait("MUZZLE") &&
            one_in(18 - dex_cur - skillLevel("unarmed")) &&
            (!wearing_something_on(bp_mouth))) {
        special_attack tmp;
        tmp.cut = 4;
        if (is_u) {
            tmp.text = string_format(_("You nip at %s!"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%s nips and harries %s!"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%s nips and harries %s!"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }
    
    if (!has_trait("FANGS") && has_trait("BEAR_MUZZLE") &&
            one_in(20 - dex_cur - skillLevel("unarmed")) &&
            (!wearing_something_on(bp_mouth))) {
        special_attack tmp;
        tmp.cut = 5;
        if (is_u) {
            tmp.text = string_format(_("You bite %s!"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%s bites %s!"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%s bites %s!"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    if (!has_trait("FANGS") && has_trait("LONG_MUZZLE") &&
            one_in(18 - dex_cur - skillLevel("unarmed")) &&
            (!wearing_something_on(bp_mouth))) {
        special_attack tmp;
        tmp.stab = 18;
        if (is_u) {
            tmp.text = string_format(_("You bite a chunk out of %s!"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%s bites a chunk out of %s!"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%s bites a chunk out of %s!"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    if (has_trait("MANDIBLES") && one_in(22 - dex_cur - skillLevel("unarmed")) &&
            (!wearing_something_on(bp_mouth))) {
        special_attack tmp;
        tmp.cut = 12;
        if (is_u) {
            tmp.text = string_format(_("You slice %s with your mandibles!"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%s slices %s with his mandibles!"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%s slices %s with her mandibles!"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    if (has_trait("BEAK") && one_in(15 - dex_cur - skillLevel("unarmed")) &&
            (!wearing_something_on(bp_mouth))) {
        special_attack tmp;
        tmp.stab = 15;
        if (is_u) {
            tmp.text = string_format(_("You peck %s!"),
                                     target.c_str());
        } else {
            tmp.text = string_format(_("%s pecks %s!"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    if (has_trait("HOOVES") && one_in(25 - dex_cur - 2 * skillLevel("unarmed"))) {
        special_attack tmp;
        tmp.bash = str_cur * 3;
        if (tmp.bash > 40) {
            tmp.bash = 40;
        }
        if (is_u) {
            tmp.text = string_format(_("You kick %s with your hooves!"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%s kicks %s with his hooves!"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%s kicks %s with her hooves!"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    if (has_trait("RAP_TALONS") && one_in(30 - dex_cur - 2 * skillLevel("unarmed"))) {
        special_attack tmp;
        tmp.cut = str_cur * 4;
        if (tmp.cut > 60) {
            tmp.cut = 60;
        }
        if (is_u) {
            tmp.text = string_format(_("You slash %s with a talon!"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%s slashes %s with a talon!"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%s slashes %s with a talon!"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    if (has_trait("HORNS") && one_in(20 - dex_cur - skillLevel("unarmed"))) {
        special_attack tmp;
        tmp.bash = 3;
        tmp.stab = 3;
        if (is_u) {
            tmp.text = string_format(_("You headbutt %s with your horns!"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%s headbutts %s with his horns!"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%s headbutts %s with her horns!"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    if (has_trait("HORNS_CURLED") && one_in(20 - dex_cur - skillLevel("unarmed"))) {
        special_attack tmp;
        tmp.bash = 14;
        if (is_u) {
            tmp.text = string_format(_("You headbutt %s with your curled horns!"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%s headbutts %s with his curled horns!"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%s headbutts %s with her curled horns!"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    if (has_trait("HORNS_POINTED") && one_in(22 - dex_cur - skillLevel("unarmed"))){
        special_attack tmp;
        tmp.stab = 24;
        if (is_u) {
            tmp.text = string_format(_("You stab %s with your pointed horns!"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%s stabs %s with his pointed horns!"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%s stabs %s with her pointed horns!"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    if (has_trait("ANTLERS") && one_in(20 - dex_cur - skillLevel("unarmed"))) {
        special_attack tmp;
        tmp.bash = 4;
        if (is_u) {
            tmp.text = string_format(_("You butt %s with your antlers!"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%s butts %s with his antlers!"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%s butts %s with her antlers!"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    if (has_trait("TAIL_STING") && one_in(3) && one_in(10 - dex_cur)) {
        special_attack tmp;
        tmp.stab = 20;
        if (is_u) {
            tmp.text = string_format(_("You sting %s with your tail!"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%s stings %s with his tail!"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%s stings %s with her tail!"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    if (has_trait("TAIL_CLUB") && one_in(3) && one_in(10 - dex_cur)) {
        special_attack tmp;
        tmp.bash = 18;
        if (is_u) {
            tmp.text = string_format(_("You club %s with your tail!"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%s clubs %s with his tail!"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%s clubs %s with her tail!"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

	    if (has_trait("TAIL_THICK") && one_in(3) && one_in(10 - dex_cur)) {
        special_attack tmp;
        tmp.bash = 8;
        if (is_u) {
            tmp.text = string_format(_("You whap %s with your tail!"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%s whaps %s with his tail!"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%s whaps %s with her tail!"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    if (has_trait("ARM_TENTACLES") || has_trait("ARM_TENTACLES_4") ||
            has_trait("ARM_TENTACLES_8")) {
        int num_attacks = 1;
        if (has_trait("ARM_TENTACLES_4")) {
            num_attacks = 3;
        }
        if (has_trait("ARM_TENTACLES_8")) {
            num_attacks = 7;
        }
        if (weapon.is_two_handed(this)) {
            num_attacks--;
        }

        for (int i = 0; i < num_attacks; i++) {
            if (one_in(18 - dex_cur - skillLevel("unarmed"))) {
                special_attack tmp;
                tmp.bash = str_cur / 2;
                if (is_u) {
                    tmp.text = string_format(_("You slap %s with your tentacle!"),
                                             target.c_str());
                } else if (male) {
                    tmp.text = string_format(_("%s slaps %s with his tentacle!"),
                                             name.c_str(), target.c_str());
                } else {
                    tmp.text = string_format(_("%s slaps %s with her tentacle!"),
                                             name.c_str(), target.c_str());
                }
                ret.push_back(tmp);
            }
        }
     }

    return ret;
}

std::string melee_message(matec_id tec_id, player &p, int bash_dam, int cut_dam, int stab_dam)
{
    if (ma_techniques.find(tec_id) != ma_techniques.end()) {
        if (ma_techniques[tec_id].messages.size() < 2) {
            return "The bugs nibble %s";
        } else if (p.is_npc()) {
            return ma_techniques[tec_id].messages[1];
        } else {
            return ma_techniques[tec_id].messages[0];
        }
    }

    // message should be based on how the weapon is used, and the total damage inflicted

    const bool npc = p.is_npc();
    // stabbing weapon or a spear
    if (p.weapon.has_flag("SPEAR") || (p.weapon.has_flag("STAB") && stab_dam > cut_dam)) {
        if (bash_dam + stab_dam + cut_dam >= 30) {
          // More variety.
          switch(rng(0, 2)) {
            case 0:
             return npc ? _("<npcname> impales %s") : _("You impale %s");
           case 1:
             return npc ? _("<npcname> gouges %s") : _("You gouge %s");
           case 2:
             return npc ? _("<npcname> runs through %s") : _("You run through %s");
          }
        } else if (bash_dam + stab_dam + cut_dam >= 20) {
            return npc ? _("<npcname> punctures %s") : _("You puncture %s");
        } else if (bash_dam + stab_dam + cut_dam >= 10) {
            return npc ? _("<npcname> pierces %s") : _("You pierce %s");
        } else {
            return npc ? _("<npcname> pokes %s") : _("You poke %s");
        }
    } else if (p.weapon.is_cutting_weapon()) {  // cutting weapon
        if (bash_dam + stab_dam + cut_dam >= 30) {
            switch(rng(0, 2)) {
              case 0:
                if (p.weapon.has_flag("STAB"))
                  return npc ? _("<npcname> guts %s") : _("You gut %s");
                else
                  return npc ? _("<npcname> chops %s") : _("You chop %s");
              case 1:
                return npc ? _("<npcname> slashes %s") : _("You slash %s");
              case 2:
                if (p.weapon.has_flag("STAB"))
                  return npc ? _("<npcname> mutilates %s") : _("You mutilate %s");
                else
                  return npc ? _("<npcname> maims %s") : _("You maim %s");
            }
        } else if (bash_dam + stab_dam + cut_dam >= 20) {
            if (p.weapon.has_flag("STAB"))
              return npc ? _("<npcname> stabs %s") : _("You stab %s");
            else
              return npc ? _("<npcname> slices %s") : _("You slice %s");
        } else if (bash_dam + stab_dam + cut_dam >= 10) {
            return npc ? _("<npcname> cuts %s") : _("You cut %s");
        } else {
            return npc ? _("<npcname> nicks %s") : _("You nick %s");
        }
    } else {  // bashing weapon (default)
        if (bash_dam + stab_dam + cut_dam >= 30) {
          switch(rng(0, 2)) {
           case 0:
             return npc ? _("<npcname> clobbers %s") : _("You clobber %s");
           case 1:
             return npc ? _("<npcname> smashes %s") : _("You smash %s");
           case 2:
             return npc ? _("<npcname> thrashes %s") : _("You thrash %s");
          }
        } else if (bash_dam + stab_dam + cut_dam >= 20) {
            return npc ? _("<npcname> batters %s") : _("You batter %s");
        } else if (bash_dam + stab_dam + cut_dam >= 10) {
            return npc ? _("<npcname> whacks %s") : _("You whack %s");
        } else {
            return npc ? _("<npcname> hits %s") : _("You hit %s");
        }
    }

    return "The bugs attack %s";
}

// display the hit message for an attack
void player_hit_message(game* g, player* attacker, std::string message,
                        std::string target_name, int dam, bool crit)
{
    std::string msg;
    if (dam <= 0) {
        if (attacker->is_npc()) {
            //~ NPC hits something but does no damage
            msg = string_format(_("%s but does no damage."), message.c_str());
        } else {
            //~ you hit something but do no damage
            msg = string_format(_("%s but do no damage."), message.c_str());
        }
    } else if (crit) {
        //~ someone hits something for %d damage (critical)
        msg = string_format(_("%s for %d damage. Critical!"),
                            message.c_str(), dam);
    } else {
        //~ someone hits something for %d damage
        msg = string_format(_("%s for %d damage."), message.c_str(), dam);
    }

    // same message is used for player and npc,
    // just using this for the <npcname> substitution.
    g->add_msg_player_or_npc(attacker, msg.c_str(), msg.c_str(),
                             target_name.c_str());
}

void melee_practice(const calendar& turn, player &u, bool hit, bool unarmed,
                    bool bashing, bool cutting, bool stabbing)
{
    int min = 2;
    int max = 2;
    std::string first = "";
    std::string second = "";
    std::string third = "";

    if (hit)
    {
        min = 5;
        max = 10;
        u.practice(turn, "melee", rng(5, 10));
    } else {
        u.practice(turn, "melee", rng(2, 5));
    }

    // type of weapon used determines order of practice
    if (u.weapon.has_flag("SPEAR"))
    {
        if (stabbing) first  = "stabbing";
        if (bashing)  second = "bashing";
        if (cutting)  third  = "cutting";
    }
    else if (u.weapon.has_flag("STAB"))
    {
        // stabbity weapons have a 50-50 chance of raising either stabbing or cutting first
        if (one_in(2))
        {
            if (stabbing) first  = "stabbing";
            if (cutting)  second = "cutting";
            if (bashing)  third  = "bashing";
        } else
        {
            if (cutting)  first  = "cutting";
            if (stabbing) second = "stabbing";
            if (bashing)  third  = "bashing";
        }
    }
    else if (u.weapon.is_cutting_weapon()) // cutting weapon
    {
        if (cutting)  first  = "cutting";
        if (bashing)  second = "bashing";
        if (stabbing) third  = "stabbing";
    }
    else // bashing weapon
    {
        if (bashing)  first  = "bashing";
        if (cutting)  second = "cutting";
        if (stabbing) third  = "stabbing";
    }

    if (unarmed) u.practice(turn, "unarmed", rng(min, max));
    if (!first.empty())  u.practice(turn, first, rng(min, max));
    if (!second.empty()) u.practice(turn, second, rng(min, max));
    if (!third.empty())  u.practice(turn, third, rng(min, max));
}

int attack_speed(player &u)
{
 int move_cost = u.weapon.attack_time() / 2;
 int skill_cost = (int)(move_cost / (pow(static_cast<float>(u.skillLevel("melee")), 3.0f)/400 +1));
 int dexbonus = (int)( pow(std::max(u.dex_cur - 8, 0), 0.8) * 3 );

 move_cost += skill_cost;
 move_cost += 20 * u.encumb(bp_torso);
 move_cost -= dexbonus;

 if (u.has_trait("LIGHT_BONES"))
  move_cost *= .9;
 if (u.has_trait("HOLLOW_BONES"))
  move_cost *= .8;

 move_cost -= u.disease_intensity("speed_boost");

 if (move_cost < 25)
  return 25;

 return move_cost;
}
