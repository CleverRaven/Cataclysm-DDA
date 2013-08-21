#include "player.h"
#include "bionics.h"
#include "debug.h"
#include "game.h"
#include "keypress.h"
#include <sstream>
#include <stdlib.h>

#include "cursesdef.h"

void hit_message(game *g, bool is_u, std::string You, std::string your, std::string verb,
                          std::string weapon, std::string target, int dam, bool crit);
void melee_practice(const calendar& turn, player &u, bool hit, bool unarmed,
                    bool bashing, bool cutting, bool stabbing);
int  attack_speed(player &u, bool missed);
int  stumble(player &u);
std::string melee_verb(technique_id tech, player &p, int bash_dam, int cut_dam, int stab_dam);

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
 return (weapon.typeId() != "null" && !weapon.is_style());
}

bool player::unarmed_attack()
{
 return (weapon.typeId() == "null" || weapon.is_style() ||
         weapon.has_flag("UNARMED_WEAPON"));
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
   numdice += int(disease_level("drunk") / 300);
  else
   numdice += int(disease_level("drunk") / 400);
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

int player::hit_mon(game *g, monster *z, bool allow_grab) // defaults to true
{
 bool is_u = (this == &(g->u));	// Affects how we'll display messages
 if (is_u)
  z->add_effect(ME_HIT_BY_PLAYER, 100); // Flag as attacked by us

 std::string You  = rm_prefix(is_u ? _("<You>You")  : string_format(_("<You>%s"), name.c_str()));
 std::string Your = rm_prefix(is_u ? _("<Your>Your") : string_format(_("<Your>%s"), name.c_str()));
 std::string your = rm_prefix(is_u ? _("<your>your") : (male ? _("<your>his") : _("<your>her")));
 std::string verb = std::string(is_u ? _("%1$s hit %4$s"):_("%1$s hits %4$s")) + "\003<%2$c%3$c>";
 std::string target = rmp_format(_("<target>the %s"), z->name().c_str());

// If !allow_grab, then we already grabbed them--meaning their dodge is hampered
 int mondodge = (allow_grab ? z->dodge_roll() : z->dodge_roll() / 3);

 bool missed = (hit_roll() < mondodge ||
                one_in(4 + dex_cur + weapon.type->m_to_hit));

 int move_cost = attack_speed(*this, missed);

 if (missed) {
  int stumble_pen = stumble(*this);
  if (is_u) {	// Only display messages if this is the player
   if (weapon.has_technique(TEC_FEINT, this))
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

// Pick one or more special attacks
 technique_id technique = pick_technique(g, z, NULL, critical_hit, allow_grab);

// Handles effects as well; not done in melee_affect_*
 perform_technique(technique, g, z, NULL, bash_dam, cut_dam, stab_dam, pain);
 if (weapon.has_technique(TEC_FLAMING, this)) { // bypass technique selection, it's on FIRE after all
   z->add_effect(ME_ONFIRE, rng(3, 4));
 }
 z->speed -= int(pain / 2);

// Mutation-based attacks
 perform_special_attacks(g, z, NULL, bash_dam, cut_dam, stab_dam);

    verb = melee_verb(technique, *this, bash_dam, cut_dam, stab_dam);

// Handles speed penalties to monster & us, etc
 melee_special_effects(g, z, NULL, critical_hit, bash_dam, cut_dam, stab_dam);

// Make a rather quiet sound, to alert any nearby monsters
 if (weapon.typeId() != "style_ninjutsu") // Ninjutsu is silent!
  g->sound(posx, posy, 8, "");

 int dam = bash_dam + (cut_dam > stab_dam ? cut_dam : stab_dam);

 if( g->u_see( z ) ) {
     hit_message(g, is_u, You, your, verb, weapon.tname(), target, dam, critical_hit);
 }

 bool bashing = (bash_dam >= 10 && !unarmed_attack());
 bool cutting = (cut_dam >= 10);
 bool stabbing = (stab_dam >= 5);
 melee_practice(g->turn, *this, true, unarmed_attack(), bashing, cutting, stabbing);

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
 bool is_u = (this == &(g->u));	// Affects how we'll display messages

 if (is_u && p.is_npc()) {
  npc* npcPtr = dynamic_cast<npc*>(&p);
  npcPtr->make_angry();
 }

 std::string You  = rm_prefix(is_u ? _("<You>You")  : string_format(_("<You>%s"), name.c_str()));
 std::string Your = rm_prefix(is_u ? _("<Your>Your") : string_format(_("<Your>%s"), name.c_str()));
 std::string your = rm_prefix(is_u ? _("<your>your") : (male ? _("<your>his") : _("<your>her")));
 std::string verb = std::string(is_u ? _("%1$s hit %4$s"):_("%1$s hits %4$s")) + "\003<%2$c%3$c>";

// Divide their dodge roll by 2 if this is a grab
 int target_dodge = (allow_grab ? p.dodge_roll(g) : p.dodge_roll(g) / 2);
 int hit_value = hit_roll() - target_dodge;
 bool missed = (hit_roll() <= 0);

 int move_cost = attack_speed(*this, missed);

 if (missed) {
  int stumble_pen = stumble(*this);
  if (is_u) {	// Only display messages if this is the player
   if (weapon.has_technique(TEC_FEINT, this))
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
  if (weapon.has_technique(TEC_FEINT, this))
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
 std::string target = p.is_npc() ? rmp_format(_("<target>%s's %s"), p.name.c_str(), bodypart.c_str()) : rmp_format(_("<target>your %s"), bodypart.c_str()) ;

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
  p.rem_disease("armor_boost");

 int pain = 0; // Boost to pain; required for perform_technique

// Pick one or more special attacks
 technique_id technique = pick_technique(g, NULL, &p, critical_hit, allow_grab);

// Handles effects as well; not done in melee_affect_*
 perform_technique(technique, g, NULL, &p, bash_dam, cut_dam, stab_dam, pain);
 if (weapon.has_technique(TEC_FLAMING, this)) { // bypass technique selection, it's on FIRE after all
   p.add_disease("onfire", rng(2, 3));
 }
 p.pain += pain;

// Mutation-based attacks
 perform_special_attacks(g, NULL, &p, bash_dam, cut_dam, stab_dam);

// Handles speed penalties to monster & us, etc
 melee_special_effects(g, NULL, &p, critical_hit, bash_dam, cut_dam, stab_dam);

// Make a rather quiet sound, to alert any nearby monsters
 if (weapon.typeId() != "style_ninjutsu") // Ninjutsu is silent!
  g->sound(posx, posy, 8, "");

 p.hit(g, bp_hit, side, bash_dam, (cut_dam > stab_dam ? cut_dam : stab_dam));

 verb = melee_verb(technique, *this, bash_dam, cut_dam, stab_dam);
 int dam = bash_dam + (cut_dam > stab_dam ? cut_dam : stab_dam);
 hit_message(g, is_u, You, your, verb, weapon.tname(), target, dam, critical_hit);

 bool bashing = (bash_dam >= 10 && !unarmed_attack());
 bool cutting = (cut_dam >= 10 && cut_dam >= stab_dam);
 bool stabbing = (stab_dam >= 10 && stab_dam >= cut_dam);
 melee_practice(g->turn, *this, true, unarmed_attack(), bashing, cutting, stabbing);

 if (dam >= 5 && has_artifact_with(AEP_SAP_LIFE))
  healall( rng(dam / 10, dam / 5) );

 if (allow_grab && technique == TEC_GRAB) {
// Move our weapon to a temp slot, if it's not unarmed
  if (p.weapon.has_technique(TEC_BREAK, &p) &&
      dice(p.dex_cur + p.skillLevel("melee"), 12) >
      dice(dex_cur + skillLevel("melee"), 10)) {
   g->add_msg_player_or_npc(&p, _("%s break the grab!"), _("%s breaks the grab!"), target.c_str());
  } else if (!unarmed_attack()) {
   item tmpweap = remove_weapon();
   hit_player(g, p, false); // False means a second grab isn't allowed
   weapon = tmpweap;
  } else
   hit_player(g, p, false); // False means a second grab isn't allowed
 }
 if (tech_def == TEC_COUNTER) {
  g->add_msg_if_player(&p, _("Counter-attack!"));
  p.hit_player(g, *this);
 }
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
 } else if (chance < 0) {
  for (int i = 0; i > weapon.type->m_to_hit; i--)
   chance /= 2;
 }
 if (rng(0, 99) < chance + 4 * disease_intensity("attack_boost"))
  num_crits++;

// Dexterity to-hit roll
// ... except sometimes we don't use dexteiry!
 int stat = dex_cur;
// Some martial arts use something else to determine hits!
 if(weapon.typeId() == "style_tiger"){
   stat = (str_cur * 2 + dex_cur) / 3;
 } else if(weapon.typeId() == "style_leopard"){
   stat = (per_cur + int_cur + dex_cur * 2) / 4;
 } else if(weapon.typeId() == "style_snake"){
   stat = (per_cur + dex_cur) / 2;
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
 } else if (chance < 3) {
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

    //Mutations
    if (has_trait("TAIL_LONG")) {ret += 2;}
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

int player::roll_bash_damage(monster *z, bool crit)
{
 int ret = 0;
 int stat = str_cur; // Which stat determines damage?
 int skill = skillLevel("bashing"); // Which skill determines damage?
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
   mindrunk = disease_level("drunk") / 600;
   maxdrunk = disease_level("drunk") / 250;
  } else {
   mindrunk = disease_level("drunk") / 900;
   maxdrunk = disease_level("drunk") / 400;
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

 if (z != NULL && z->has_flag(MF_PLASTIC))
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
  if (z != NULL)
   ret -= z->armor_bash() / 2;
 } else if (z != NULL)
  ret -= z->armor_bash();

 return (ret < 0 ? 0 : ret);
}

int player::roll_cut_damage(monster *z, bool crit)
{
 if (weapon.has_flag("SPEAR"))
  return 0;  // Stabs, doesn't cut!
 int z_armor_cut = (z == NULL ? 0 : z->armor_cut() - skillLevel("cutting") / 2);

 if (crit)
  z_armor_cut /= 2;
 if (z_armor_cut < 0)
  z_armor_cut = 0;

 double ret = weapon.damage_cut() - z_armor_cut;

 if (unarmed_attack() && !wearing_something_on(bp_hands)) {
  if (has_trait("CLAWS"))
   ret += 6;
  if (has_trait("TALONS"))
   ret += 6 + ((int)skillLevel("unarmed") > 8 ? 8 : (int)skillLevel("unarmed"));
  if (has_trait("SLIME_HANDS") && (z == NULL || !z->has_flag(MF_ACIDPROOF)))
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

int player::roll_stab_damage(monster *z, bool crit)
{
 double ret = 0;
 int z_armor = (z == NULL ? 0 : z->armor_cut() - 3 * skillLevel("stabbing"));

 if (crit)
  z_armor /= 3;
 if (z_armor < 0)
  z_armor = 0;

 if (unarmed_attack() && !wearing_something_on(bp_hands)) {
  ret = 0 - z_armor;
  if (has_trait("CLAWS"))
   ret += 6;
  if (has_trait("NAILS") && z_armor == 0)
   ret++;
  if (has_trait("THORNS"))
   ret += 4;
 } else if (weapon.has_flag("SPEAR") || weapon.has_flag("STAB"))
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
int player::roll_stuck_penalty(monster *z, bool stabbing)
{
    // The cost of the weapon getting stuck, in units of move points.
    const int weapon_speed = attack_speed( *this, false );
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

technique_id player::pick_technique(game *g, monster *z, player *p,
                                    bool crit, bool allowgrab)
{
 if (z == NULL && p == NULL)
  return TEC_NULL;

 std::vector<technique_id> possible;
 bool downed = ((z && !z->has_effect(ME_DOWNED)) ||
                (p && !p->has_disease("downed"))  );
 int base_str_req = 0;

 if (z)
  base_str_req = z->type->size;
 else if (p)
  base_str_req = 1 + (2 + p->str_cur) / 4;

 if (allowgrab) { // Check if grabs AREN'T REALLY ALLOWED
  if (z && z->has_flag(MF_PLASTIC))
   allowgrab = false;
 }

 if (crit) { // Some are crit-only

  if (weapon.has_technique(TEC_SWEEP, this) &&
      (!z || !z->has_flag(MF_FLIES)) && !downed)
   possible.push_back(TEC_SWEEP);

  if (weapon.has_technique(TEC_PRECISE, this))
   possible.push_back(TEC_PRECISE);

  if (weapon.has_technique(TEC_BRUTAL, this) && !downed &&
      str_cur + skillLevel("melee") >= 4 + base_str_req)
   possible.push_back(TEC_BRUTAL);

 }

 if (possible.empty()) { // Use non-crits only if any crit-onlies aren't used

  if (weapon.has_technique(TEC_DISARM, this) && !z &&
      p->weapon.typeId() != "null" && !p->weapon.has_flag("UNARMED_WEAPON") &&
      dice(   dex_cur +    skillLevel("unarmed"),  8) >
      dice(p->dex_cur + p->skillLevel("melee"),   10))
   possible.push_back(TEC_DISARM);

  if (weapon.has_technique(TEC_GRAB, this) && allowgrab)
   possible.push_back(TEC_GRAB);

  if (weapon.has_technique(TEC_RAPID, this))
   possible.push_back(TEC_RAPID);

  if (weapon.has_technique(TEC_THROW, this) && !downed &&
      str_cur + skillLevel("melee") >= 4 + base_str_req * 4 + rng(-4, 4))
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
      if (g->active_npc[npcdex]->attitude == NPCATT_KILL)
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
 std::string You = rm_prefix(is_npc() ? string_format(_("<You>%s"), name.c_str()) : _("<You>You"));
 std::string target = rm_prefix(mon ? string_format("<target>the %s",z->name().c_str()) :
                       (p->is_npc() ? string_format(_("<target>%s"), p->name.c_str()) : "<target>you"));
 int tarx = (mon ? z->posx : p->posx), tary = (mon ? z->posy : p->posy);

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
  } else if (p != NULL && p->weapon.typeId() != "style_judo") {
   p->add_disease("downed", rng(1, 2));
   bash_dam += 3;
  }
  break;

 case TEC_PRECISE:
  if (z != NULL)
   z->add_effect(ME_STUNNED, rng(1, 4));
  else if (p != NULL)
   p->add_disease("stunned", rng(1, 2));
  pain += rng(5, 8);
  break;

 case TEC_BRUTAL:
  if (z != NULL) {
   z->add_effect(ME_STUNNED, 1);
   z->knock_back_from(g, posx, posy);
  } else if (p != NULL) {
   p->add_disease("stunned", 1);
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
   if (p->weapon.typeId() != "style_judo")
    p->add_disease("downed", rng(1, 2));
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
         if (g->z[mondex].hurt(dam)) {
             g->z[mondex].die(g);
         }
         if (weapon.has_technique(TEC_FLAMING, this))  { // Add to wide attacks
             g->z[mondex].add_effect(ME_ONFIRE, rng(3, 4));
         }
         std::string temp_target = rmp_format(_("<target>the %s"), g->z[mondex].name().c_str());
         g->add_msg_player_or_npc( this, _("You hit %s!"), _("<npcname> hits %s!"), temp_target.c_str() );
     }
     int npcdex = g->npc_at(x, y);
     if (npcdex != -1 &&
         hit_roll() >= rng(0, 5) + g->active_npc[npcdex]->dodge_roll(g)) {
         count_hit++;
         int dam = roll_bash_damage(NULL, false);
         int cut = roll_cut_damage (NULL, false);
         g->active_npc[npcdex]->hit(g, bp_legs, 3, dam, cut);
         if (weapon.has_technique(TEC_FLAMING, this)) {// Add to wide attacks
             g->active_npc[npcdex]->add_disease("onfire", rng(2, 3));
         }
         std::string temp_target = rmp_format(_("<target>%s"), g->active_npc[npcdex]->name.c_str());
         g->add_msg_player_or_npc( this, _("You hit %s!"), _("<npcname> hits %s!"), temp_target.c_str() );

         g->active_npc[npcdex]->add_disease("onfire", rng(2, 3));
     }
    }
   }
  }
   g->add_msg_if_player(p, ngettext("%d enemy hit!", "%d enemies hit!", count_hit), count_hit);
 } break;

 case TEC_DISARM:
  g->m.add_item_or_charges(p->posx, p->posy, p->remove_weapon());
  g->add_msg_player_or_npc( this, _("You disarm %s!"), _("<npcname> disarms %s!"), target.c_str() );
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
  foe_melee_skill = p->dex_cur + p->skillLevel("melee");

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
     dice(dex_cur + skillLevel("melee"), 12) > dice(foe_melee_skill, 10))
  return TEC_WBLOCK_3;

 if (weapon.has_technique(TEC_WBLOCK_2) &&
     dice(dex_cur + skillLevel("melee"), 6) > dice(foe_melee_skill, 10))
  return TEC_WBLOCK_2;

 if (weapon.has_technique(TEC_WBLOCK_1) &&
     dice(dex_cur + skillLevel("melee"), 3) > dice(foe_melee_skill, 10))
  return TEC_WBLOCK_1;

 if (weapon.has_technique(TEC_DEF_DISARM, this) &&
     z == NULL && p->weapon.typeId() != "null" &&
     !p->weapon.has_flag("UNARMED_WEAPON") &&
     dice(   dex_cur +    skillLevel("unarmed"), 8) >
     dice(p->dex_cur + p->skillLevel("melee"),  10))
  return TEC_DEF_DISARM;

 if (weapon.has_technique(TEC_DEF_THROW, this) &&
     str_cur + skillLevel("melee") >= foe_size + rng(-4, 4) &&
     hit_roll() > rng(1, 5) + foe_dodge && !one_in(3))
  return TEC_DEF_THROW;

 if (weapon.has_technique(TEC_COUNTER, this) &&
     hit_roll() > rng(1, 10) + foe_dodge && !one_in(3))
  return TEC_COUNTER;

 if (weapon.has_technique(TEC_BLOCK_LEGS, this) &&
     (hp_cur[hp_leg_l] >= 20 || hp_cur[hp_leg_r] >= 20) &&
     dice(dex_cur + skillLevel("unarmed") + skillLevel("melee"), 13) >
     dice(8 + foe_melee_skill, 10))
  return TEC_BLOCK_LEGS;

 if (weapon.has_technique(TEC_BLOCK, this) &&
     (hp_cur[hp_arm_l] >= 20 || hp_cur[hp_arm_r] >= 20) &&
     dice(dex_cur + skillLevel("unarmed") + skillLevel("melee"), 16) >
     dice(6 + foe_melee_skill, 10))
  return TEC_BLOCK;

 blocks_left++; // We didn't use any blocks, so give it back!
 return TEC_NULL;
}

void player::perform_defensive_technique(
  technique_id technique, game *g, monster *z, player *p,
  body_part &bp_hit, int &side, int &bash_dam, int &cut_dam, int &stab_dam)

{
 bool mon = (z != NULL);
 std::string You = rm_prefix(is_npc() ? string_format(_("<You>%s"), name.c_str()) : _("<You>You"));
 std::string your = rm_prefix(is_npc() ? (male ? _("<your>his") : _("<your>her")) : _("<your>your"));
 std::string target = rm_prefix(mon ? string_format(_("<target>the %s"),z->name().c_str()) : string_format(_("<target>%s"),p->name.c_str()));

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
   g->add_msg_player_or_npc( this, _("You block with your %s!"), _("<npcname> blocks with their %s!"),
                             body_part_name(bp_hit, side).c_str() );

   bash_dam *= .5;
   double reduction = 1.0;
// Special reductions for certain styles
   if (weapon.typeId() == "style_tai_chi")
    reduction -= double(0.08 * double(per_cur - 6));
   if (weapon.typeId() == "style_taekwondo")
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
   g->add_msg_player_or_npc( this, _("You block with your %s!"), _("<npcname> blocks with their %s!"),
                             weapon.tname().c_str() );

  case TEC_COUNTER:
   break; // Handled elsewhere

  case TEC_DEF_THROW:
   g->add_msg_player_or_npc( this, _("You throw %s."), _("<npcname> throws %s."), target.c_str() );
   bash_dam = 0;
   cut_dam  = 0;
   stab_dam = 0;
   if (mon) {
    z->add_effect(ME_DOWNED, rng(1, 2));
    z->knock_back_from(g, posx + rng(-1, 1), posy + rng(-1, 1));
   } else {
    p->add_disease("downed", rng(1, 2));
    p->knock_back_from(g, posx + rng(-1, 1), posy + rng(-1, 1));
   }
   break;

  case TEC_DEF_DISARM:
   g->m.add_item_or_charges(p->posx, p->posy, p->remove_weapon());
// Re-roll damage, without our weapon
   bash_dam = p->roll_bash_damage(NULL, false);
   cut_dam  = p->roll_cut_damage(NULL, false);
   stab_dam = p->roll_stab_damage(NULL, false);
   g->add_msg_player_or_npc( this, _("You disarm %s."), _("<npcname> disarms %s."), target.c_str() );

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
 std::string target;
 if(z!=NULL) target = rmp_format(_("<target>the %s"),z->name().c_str());
 else if(p!=NULL) target = rmp_format(_("<target>%s"),p->name.c_str());
 else target = "";

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
  if (z != NULL) {
   if (!z->has_effect(ME_POISONED))
    g->add_msg_if_player(p,_("You poison %s!"), target.c_str());
   z->add_effect(ME_POISONED, 6);
  } else if (p != NULL) {
   if (!p->has_disease("poison"))
    g->add_msg_if_player(p,_("You poison %s!"), target.c_str());
   p->add_disease("poison", 6);
  }
 }
}

void player::melee_special_effects(game *g, monster *z, player *p, bool crit,
                                   int &bash_dam, int &cut_dam, int &stab_dam)
{
 if (z == NULL && p == NULL)
  return;
 bool mon = (z != NULL);
 bool is_u = (!is_npc());
 std::string You  = rm_prefix(is_u ? _("<You>You")  : string_format(_("<You>%s"), name.c_str()));
 std::string Your = rm_prefix(is_u ? _("<Your>Your") : string_format(_("<Your>%s's"), name.c_str()));
 std::string your = rm_prefix(is_u ? _("<your>your") : (male ? _("<your>his") : _("<your>her")));
 std::string target = rm_prefix(mon ? string_format(_("<target>the %s"),z->name().c_str()) :
                       (p->is_npc() ? string_format(_("<target>%s"), p->name.c_str()) : _("<target>you")));
 std::string target_possessive = rm_prefix(mon ? string_format(_("<target's>the %s's"), z->name().c_str()) :
                                  (p->is_npc() ? string_format(_("<target's>%s's"), p->name.c_str()) : "<target's>your"));
 int tarposx = (mon ? z->posx : p->posx), tarposy = (mon ? z->posy : p->posy);

// Bashing effecs
 if (mon)
  z->moves -= rng(0, bash_dam * 2);
 else
  p->moves -= rng(0, bash_dam * 2);

// Bashing crit
 if (crit && !unarmed_attack()) {
  int turns_stunned = int(bash_dam / 20) + rng(0, int(skillLevel("bashing") / 2));
  if (turns_stunned > 6)
   turns_stunned = 6;
  if (turns_stunned > 0) {
   if (mon)
    z->add_effect(ME_STUNNED, turns_stunned);
   else
    p->add_disease("stunned", 1 + turns_stunned / 2);
  }
 }

// Stabbing effects
 int stab_moves = rng(stab_dam / 2, stab_dam * 1.5);
 if (crit)
  stab_moves *= 1.5;
 if (stab_moves >= 150) {
     g->add_msg_player_or_npc( p, _("You force %s to the ground!"),
                               _("<npcname> forces %s to the ground!"), target.c_str() );

     if (mon) {
         z->add_effect(ME_DOWNED, 1);
         z->moves -= stab_moves / 2;
     } else {
         p->add_disease("downed", 1);
         p->moves -= stab_moves / 2;
     }
 } else if (mon) {
     z->moves -= stab_moves;
 } else {
     p->moves -= stab_moves;
 }

// Bonus attacks!
 bool shock_them = (has_active_bionic("bio_shock") && power_level >= 2 &&
                    (unarmed_attack() || weapon.made_of("iron") ||
                     weapon.made_of("steel") ||weapon.made_of("silver")) &&
                    (!mon || !z->has_flag(MF_ELECTRIC)) && one_in(3));

 bool drain_them = (has_active_bionic("bio_heat_absorb") && power_level >= 1 &&
                    !is_armed() && (!mon || z->has_flag(MF_WARM)));

 drain_them &= one_in(2);	// Only works half the time

 if (drain_them)
  power_level--;

 if (shock_them) {
  power_level -= 2;
  int shock = rng(2, 5);
  if (mon) {
   z->hurt( shock * rng(1, 3) );
   z->moves -= shock * 180;
   g->add_msg_player_or_npc( p, _("You shock %s."), _("<npcname> shocks %s."), target.c_str() );
  } else {
   p->hurt(g, bp_torso, 0, shock * rng(1, 3));
   p->moves -= shock * 80;
  }
 }

 if (drain_them) {
  charge_power(rng(0, 2));
  g->add_msg_player_or_npc( p, _("You drain %s body heat!"), _("<npcname> drains %s body heat!"),
                            target_possessive.c_str() );
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
  g->add_msg_if_player(p, _("Contact with %s shocks you!"), target.c_str());
 }

// Glass weapons shatter sometimes
 if (weapon.made_of("glass") &&
     rng(0, weapon.volume() + 8) < weapon.volume() + str_cur) {
     g->add_msg_player_or_npc( p, _("Your %s shatters!"), _("<npcname>'s %s shatters!"),
                               weapon.tname(g).c_str() );

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
 int cutting_penalty = roll_stuck_penalty(z, stab_dam > cut_dam);
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
 if (!unarmed_attack() && cutting_penalty > dice(str_cur * 2, 20)) {
   g->add_msg_if_player(p,_("Your %s gets stuck in %s, pulling it out of your hands!"),
              weapon.tname().c_str(), target.c_str());
  if (mon) {
   if (weapon.has_flag("SPEAR") || weapon.has_flag("STAB"))
    z->speed *= .7;
   else
    z->speed *= .85;
   z->add_item(remove_weapon());
  } else
   g->m.add_item_or_charges(posx, posy, remove_weapon());
 } else {
  if (mon && (cut_dam >= z->hp || stab_dam >= z->hp)) {
   cutting_penalty /= 2;
   cutting_penalty -= rng(skillLevel("cutting"), skillLevel("cutting") * 2 + 2);
  }
  if (cutting_penalty > 0)
   moves -= cutting_penalty;
  if (cutting_penalty >= 50)
   g->add_msg_if_player(p,_("Your %s gets stuck in %s, but you yank it free."),
              weapon.tname().c_str(), target.c_str());
  if (mon && (weapon.has_flag("SPEAR") || weapon.has_flag("STAB")))
   z->speed *= .9;
 }

// Finally, some special effects for martial arts
 if(weapon.typeId() == "style_karate"){
   dodges_left++;
   blocks_left += 2;
 } else if(weapon.typeId() == "style_aikido"){
   bash_dam /= 2;
 } else if(weapon.typeId() == "style_capoeira"){
   add_disease("dodge_boost", 2, 2);
 } else if(weapon.typeId() == "style_muay_thai"){
   if ((mon && z->type->size >= MS_LARGE) || (!mon && p->str_max >= 12))
    bash_dam += rng((mon ? z->type->size : (p->str_max - 8) / 4),
                    3 * (mon ? z->type->size : (p->str_max - 8) / 4));
 } else if(weapon.typeId() == "style_tiger"){
   add_disease("damage_boost", 2, 2, 10);
 } else if(weapon.typeId() == "style_centipede"){
   add_disease("speed_boost", 2, 4, 40);
 } else if(weapon.typeId() == "style_venom_snake"){
   if (has_disease("viper_combo")) {
    if (disease_intensity("viper_combo") == 1) {
     g->add_msg_if_player(p,"Snakebite!");
     int dambuf = bash_dam;
     bash_dam = stab_dam;
     stab_dam = dambuf;
     add_disease("viper_combo", 2, 1, 2); // Upgrade to Viper Strike
    } else if (disease_intensity("viper_combo") == 2) {
     if (hp_cur[hp_arm_l] >= hp_max[hp_arm_l] * .75 &&
         hp_cur[hp_arm_r] >= hp_max[hp_arm_r] * .75   ) {
      g->add_msg_if_player(p,"Viper STRIKE!");
      bash_dam *= 3;
     } else
      g->add_msg_if_player(p,_("Your injured arms prevent a viper strike!"));
     rem_disease("viper_combo");
    }
   } else if (crit) {
    g->add_msg_if_player(p,_("Tail whip!  Viper Combo Intiated!"));
    bash_dam += 5;
    add_disease("viper_combo", 2, 1, 2);
   }
 } else if(weapon.typeId() == "style_scorpion"){
   if (crit) {
    g->add_msg_if_player(p,_("Stinger Strike!"));
    if (mon) {
     z->add_effect(ME_STUNNED, 3);
     int zposx = z->posx, zposy = z->posy;
     z->knock_back_from(g, posx, posy);
     if (z->posx != zposx || z->posy != zposy)
      z->knock_back_from(g, posx, posy); // Knock a 2nd time if the first worked
    } else {
     p->add_disease("stunned", 2);
     int pposx = p->posx, pposy = p->posy;
     p->knock_back_from(g, posx, posy);
     if (p->posx != pposx || p->posy != pposy)
      p->knock_back_from(g, posx, posy); // Knock a 2nd time if the first worked
    }
   }
 } else if(weapon.typeId() == "style_zui_quan"){
   dodges_left = 50; // Basically, unlimited.
 }
}

std::vector<special_attack> player::mutation_attacks(monster *z, player *p)
{
 std::vector<special_attack> ret;

 if (z == NULL && p == NULL)
  return ret;

 bool mon = (z != NULL);
 bool is_u = (!is_npc());// Affects how we'll display messages
 std::string You  = rm_prefix(is_u ? _("<You>You")  : string_format(_("<You>%s"), name.c_str()));
 std::string Your = rm_prefix(is_u ? _("<Your>Your") : string_format("<Your>%s's", name.c_str()));
 std::string your = rm_prefix(is_u ? _("<your>your") : (male ? _("<your>his") : _("<your>her")));
 std::string target = rm_prefix(mon ? string_format(_("<target>the %s"), z->name().c_str()) : string_format(_("<target>%s"), p->name.c_str()));

 std::stringstream text;

 if (has_trait("FANGS") && !wearing_something_on(bp_mouth) &&
     one_in(20 - dex_cur - skillLevel("unarmed"))) {
  special_attack tmp;
  text << string_format((is_u ? _("%s sink %s fangs into %s!") : _("%s sinks %s fangs into %s!")), You.c_str(), your.c_str(), target.c_str());
  tmp.text = text.str();
  tmp.stab = 20;
  ret.push_back(tmp);
 }

 if (has_trait("MANDIBLES") && one_in(22 - dex_cur - skillLevel("unarmed"))) {
  special_attack tmp;
  text << string_format((is_u ? _("%s slice %s with %s mandibles!") : _("%s slices %s with %s mandibles!")), You.c_str(), target.c_str(), your.c_str());
  tmp.text = text.str();
  tmp.cut = 12;
  ret.push_back(tmp);
 }

 if (has_trait("BEAK") && one_in(15 - dex_cur - skillLevel("unarmed"))) {
  special_attack tmp;
  text << string_format((is_u ? _("%s peck %s!") : _("%s pecks %s!")), You.c_str(), target.c_str());
  tmp.text = text.str();
  tmp.stab = 15;
  ret.push_back(tmp);
 }

 if (has_trait("HOOVES") && one_in(25 - dex_cur - 2 * skillLevel("unarmed"))) {
  special_attack tmp;
  text << string_format((is_u ? _("%s kick %s with %s hooves!") : _("%s kicks %s with %s hooves!")), You.c_str(), target.c_str(), your.c_str());
  tmp.text = text.str();
  tmp.bash = str_cur * 3;
  if (tmp.bash > 40)
   tmp.bash = 40;
  ret.push_back(tmp);
 }

 if (has_trait("HORNS") && one_in(20 - dex_cur - skillLevel("unarmed"))) {
  special_attack tmp;
  text << string_format((is_u ? _("%s headbutt %s with %s horns!") : _("%s headbutts %s with %s horns!")), You.c_str(), target.c_str(), your.c_str());
  tmp.text = text.str();
  tmp.bash = 3;
  tmp.stab = 3;
  ret.push_back(tmp);
 }

 if (has_trait("HORNS_CURLED") && one_in(20 - dex_cur - skillLevel("unarmed"))) {
  special_attack tmp;
  text << string_format((is_u ? _("%s headbutt %s with %s curled horns!") : _("%s headbutts %s with %s curled horns!")), You.c_str(), target.c_str(), your.c_str());
  tmp.text = text.str();
  tmp.bash = 14;
  ret.push_back(tmp);
 }

 if (has_trait("HORNS_POINTED") && one_in(22 - dex_cur - skillLevel("unarmed"))){
  special_attack tmp;
  text << string_format((is_u ? _("%s stab %s with %s pointed horns!") : _("%s stabs %s with %s pointed horns!")), You.c_str(), target.c_str(), your.c_str());
  tmp.text = text.str();
  tmp.stab = 24;
  ret.push_back(tmp);
 }

 if (has_trait("ANTLERS") && one_in(20 - dex_cur - skillLevel("unarmed"))) {
  special_attack tmp;
  text << string_format((is_u ? _("%s butt %s with %s antlers!") : _("%s butts %s with %s antlers!")), You.c_str(), target.c_str(), your.c_str());
  tmp.text = text.str();
  tmp.bash = 4;
  ret.push_back(tmp);
 }

 if (has_trait("TAIL_STING") && one_in(3) && one_in(10 - dex_cur)) {
  special_attack tmp;
  text << string_format((is_u ? _("%s sting %s with %s tail!") : _("%s stings %s with %s tail!")), You.c_str(), target.c_str(), your.c_str());
  tmp.text = text.str();
  tmp.stab = 20;
  ret.push_back(tmp);
 }

 if (has_trait("TAIL_CLUB") && one_in(3) && one_in(10 - dex_cur)) {
  special_attack tmp;
  text << string_format((is_u ? _("%s hit %s with %s tail!") : _("%s hits %s with %s tail!")), You.c_str(), target.c_str(), your.c_str());
  tmp.text = text.str();
  tmp.bash = 18;
  ret.push_back(tmp);
 }

 if (has_trait("ARM_TENTACLES") || has_trait("ARM_TENTACLES_4") ||
     has_trait("ARM_TENTACLES_8")) {
  int num_attacks = 1;
  if (has_trait("ARM_TENTACLES_4"))
   num_attacks = 3;
  if (has_trait("ARM_TENTACLES_8"))
   num_attacks = 7;
  if (weapon.is_two_handed(this))
   num_attacks--;

  for (int i = 0; i < num_attacks; i++) {
   if (one_in(18 - dex_cur - skillLevel("unarmed"))) {
    special_attack tmp;
    text.str("");
    text << string_format((is_u ? _("%s slap %s with %s tentacle!") : _("%s slaps %s with %s tentacle!")), You.c_str(), target.c_str(), your.c_str());
    tmp.text = text.str();
    tmp.bash = str_cur / 2;
    ret.push_back(tmp);
   }
  }
 }

 return ret;
}

std::string melee_verb(technique_id tech, player &p, int bash_dam, int cut_dam, int stab_dam)
{
 if (tech != TEC_NULL && p.weapon.is_style() &&
     p.weapon.style_data(tech).name != "")
  return (p.is_npc()?p.weapon.style_data(tech).verb_npc:p.weapon.style_data(tech).verb_you) + "\003<%2$c%3$c>";

 std::stringstream ret;

 //1$ You 2$ your 3$ weapon 4$ target
 switch (tech) {

  case TEC_SWEEP:
   return p.is_npc()? _("%1$s sweeps %2$s %3$s at %4$s") : _("%1$s sweep %2$s %3$s at %4$s");
   break;

  case TEC_PRECISE:
   return p.is_npc()? _("%1$s jabs %2$s %3$s at %4$s") : _("%1$s jab %2$s %3$s at %4$s");
   break;

  case TEC_BRUTAL:
   return p.is_npc()? _("%1$s slams %2$s %3$s against %4$s") : _("%1$s slam %2$s %3$s against %4$s");
   break;

  case TEC_GRAB:
   return p.is_npc()? _("%1$s wraps %2$s %3$s around %4$s") : _("%1$s wrap %2$s %3$s around %4$s");
   break;

  case TEC_WIDE:
   return p.is_npc()? _("%1$s swings %2$s %3$s wide at %4$s") : _("%1$s swing %2$s %3$s wide at %4$s");
   break;

  case TEC_THROW:
   return p.is_npc()? _("%1$s uses %2$s %3$s to toss %4$s") : _("%1$s use %2$s %3$s to toss %4$s");
   break;

    default: // No tech, so check our damage levels
        // verb should be based on how the weapon is used, and the total damage inflicted

        // if it's a stabbing weapon or a spear
        if (p.weapon.has_flag("SPEAR") || (p.weapon.has_flag("STAB") && stab_dam > cut_dam))
        {
            if (bash_dam + stab_dam + cut_dam >= 30)
                ret << (p.is_npc()?_("%1$s impales %4$s"):_("%1$s impale %4$s"));
            else if (bash_dam + stab_dam + cut_dam >= 20)
                ret << (p.is_npc()?_("%1$s pierces %4$s"):_("%1$s pierce %4$s"));
            else if (bash_dam + stab_dam + cut_dam >= 10)
                ret << (p.is_npc()?_("%1$s stabs %4$s"):_("%1$s stab %4$s"));
            else ret << (p.is_npc()?_("%1$s pokes %4$s"):_("%1$s poke %4$s"));
        } else if (p.weapon.is_cutting_weapon())    // if it's a cutting weapon
        {
            if (bash_dam + stab_dam + cut_dam >= 30)
                ret << (p.is_npc()?_("%1$s hacks %4$s"):_("%1$s hack %4$s"));
            else if (bash_dam + stab_dam + cut_dam >= 20)
                ret << (p.is_npc()?_("%1$s slices %4$s"):_("%1$s slice %4$s"));
            else if (bash_dam + stab_dam + cut_dam >= 10)
                ret << (p.is_npc()?_("%1$s cuts %4$s"):_("%1$s cut %4$s"));
            else ret << (p.is_npc()?_("%1$s nicks %4$s"):_("%1$s nick %4$s"));
        } else                                      // it must be a bashing weapon
        {
            if (bash_dam + stab_dam + cut_dam >= 30)
                ret << (p.is_npc()?_("%1$s clobbers %4$s"):_("%1$s clobber %4$s"));
            else if (bash_dam + stab_dam + cut_dam >= 20)
                ret << (p.is_npc()?_("%1$s batters %4$s"):_("%1$s batter %4$s"));
            else if (bash_dam + stab_dam + cut_dam >= 10)
                ret << (p.is_npc()?_("%1$s whacks %4$s"):_("%1$s whack %4$s"));
            else ret << (p.is_npc()?_("%1$s hits %4$s"):_("%1$s hit %4$s"));
        }
        ret << "\003<%2$c%3$c>";
        return ret.str();
 } // switch (tech)

 return ret.str();
}

void hit_message(game *g, bool is_u, std::string You, std::string your, std::string verb,
                          std::string weapon, std::string target, int dam, bool crit)
{
    //1$ You 2$ your 3$ weapon 4$ target
    std::string part1 = string_format(verb.c_str(), You.c_str(), your.c_str(), weapon.c_str(), target.c_str());
    std::string part2;
    if (dam <= 0) {
        part2 = is_u? _(" but do no damage."): _(" but does no damage.");
    } else {
        part2 = string_format(_(" for %d damage."), dam);
        if(crit) part1 = _("Critical! ") + part1;
    }
    g->add_msg((part1+part2).c_str());
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

int attack_speed(player &u, bool missed)
{
 int move_cost = u.weapon.attack_time() + 20 * u.encumb(bp_torso);
 if (u.has_trait("LIGHT_BONES"))
  move_cost *= .9;
 if (u.has_trait("HOLLOW_BONES"))
  move_cost *= .8;

 move_cost -= u.disease_intensity("speed_boost");

 if (move_cost < 25)
  return 25;

 return move_cost;
}
