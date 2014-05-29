#include "player.h"
#include "bionics.h"
#include "debug.h"
#include "game.h"
#include "martialarts.h"
#include "messages.h"
#include <sstream>
#include <stdlib.h>
#include <algorithm>

#include "cursesdef.h"

void player_hit_message(player* attacker, std::string message,
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
 *   skills, torso encumbrance penalties and drunken master bonuses.
 */

bool player::is_armed()
{
 return (weapon.typeId() != "null");
}

bool player::handle_melee_wear() {
// Here is where we handle wear and tear on things we use as melee weapons or shields.
    std::stringstream dump;
    int material_factor = 1;
    int damage_chance = dex_cur + ( 2 * skillLevel("melee") ) + ( 128 / std::max(str_cur,1) );
  // UNBREAKABLE_MELEE items can't be damaged through melee combat usage.
  if ((!weapon.has_flag("UNBREAKABLE_MELEE")) && (is_armed())) {
    // Here we're checking the weapon's material(s) and using the best one to determine how durable it is.
    if (weapon.made_of("plastic")) {
                material_factor = 2;
    }
    if (weapon.made_of("leather")) {
                material_factor = 3;
    }
    if (weapon.made_of("bone") || weapon.made_of("chitin") || weapon.made_of("wood")) {
                material_factor = 4;
    }
    if (weapon.made_of("stone") || weapon.made_of("silver") || weapon.made_of("gold") || weapon.made_of("lead")) {
                material_factor = 6;
    }
    if (weapon.made_of("iron") || weapon.made_of("kevlar") || weapon.made_of("aluminum")) {
                material_factor = 8;
    }
    if (weapon.made_of("steel") ) {
                material_factor = 10;
    }
    if (weapon.made_of("hardsteel")) {
                material_factor = 12;
    }
    if (weapon.made_of("ceramic")) {
                material_factor = 40;
    }
    if (weapon.made_of("superalloy") || weapon.made_of("diamond")){
                material_factor = 100;
    }
    // DURABLE_MELEE items are made to hit stuff and they do it well, so they're considered to be a lot tougher
    // than other weapons made of the same materials.
    if (weapon.has_flag("DURABLE_MELEE")) {
                material_factor *= 4;
    }
    // The weapon's current state of damage can make it more susceptible to further damage.
    damage_chance -= weapon.damage * 6;

    if (damage_chance < 2) {
        damage_chance = 2;
    }

    damage_chance *= material_factor;

    if (weapon.damage < 4 && one_in(damage_chance) && (!weapon.has_flag("UNBREAKABLE_MELEE"))){
     weapon.damage++;
     add_msg_player_or_npc( m_bad, _("Your %s is damaged by the force of the blow!"),
                                   _("<npcname>'s %s is damaged by the force of the blow!"),
                                   weapon.name.c_str());
    } else if (weapon.damage >= 4 && one_in(damage_chance) && (!weapon.has_flag("UNBREAKABLE_MELEE"))){
      add_msg_player_or_npc( m_bad, _("Your %s is destroyed by the blow!"),
      _("<npcname>'s %s is destroyed by the blow!"),
      weapon.name.c_str());
  // Dump its contents on the ground
  for (size_t i = 0; i < weapon.contents.size(); i++)
   g->m.add_item_or_charges(posx, posy, weapon.contents[i]);
   remove_weapon();
      }
  }
  return true;
}

bool player::unarmed_attack() {
 return (weapon.typeId() == "null" || weapon.has_flag("UNARMED_WEAPON"));
}

// TODO: this is here for newcharacter.cpp only, swap it out when possible
int player::base_to_hit(bool real_life, int stat)
{
 if (stat == -999)
  stat = (real_life ? dex_cur : dex_max);
 return 1 + int(stat / 2) + skillLevel("melee");
}


int player::get_hit_base()
{
    int best_bonus = 0;

    int unarmed_skill = skillLevel("unarmed");
    int bashing_skill = skillLevel("bashing");
    int cutting_skill = skillLevel("cutting");
    int stabbing_skill = skillLevel("stabbing");
    int melee_skill = skillLevel("melee");

    if (has_active_bionic("bio_cqb")) {
        unarmed_skill = 5;
        bashing_skill = 5;
        cutting_skill = 5;
        stabbing_skill = 5;
        melee_skill = 5;
    }

    // Are we unarmed?
    if (unarmed_attack()) {
        best_bonus = unarmed_skill;
        if (unarmed_skill > 4)
            best_bonus += unarmed_skill - 4; // Extra bonus for high levels
    }

    // Using a bashing weapon?
    if (weapon.is_bashing_weapon()) {
        int bash_bonus = int(bashing_skill / 3);
        if (bash_bonus > best_bonus)
            best_bonus = bash_bonus;
    }

    // Using a cutting weapon?
    if (weapon.is_cutting_weapon()) {
        int cut_bonus = int(cutting_skill / 2);
        if (cut_bonus > best_bonus)
            best_bonus = cut_bonus;
    }

    // Using a spear?
    if (weapon.has_flag("SPEAR") || weapon.has_flag("STAB")) {
        int stab_bonus = int(stabbing_skill / 2);
        if (stab_bonus > best_bonus)
            best_bonus = stab_bonus;
    }

    // Creature::get_hit_base includes stat calculations already
    return Creature::get_hit_base() + melee_skill + best_bonus;
}

int player::hit_roll()
{
// apply martial arts bonuses

 int numdice = get_hit();

 int sides = 10 - encumb(bp_torso);
 int best_bonus = 0;
 if (sides < 2)
  sides = 2;

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

// Melee calculation is in parts. This sets up the attack, then in deal_melee_attack,
// we calculate if we would hit. In Creature::deal_melee_hit, we calculate if the target dodges.
void player::melee_attack(Creature &t, bool allow_special, matec_id force_technique) {
    bool is_u = (this == &(g->u)); // Affects how we'll display messages
    if (!t.is_player()) {
        // @todo Per-NPC tracking? Right now monster hit by either npc or player will draw aggro...
        t.add_effect("hit_by_player", 100); // Flag as attacked by us for AI
    }

    std::string message = is_u ? _("You hit %s") : _("<npcname> hits %s");
    std::string target_name = t.disp_name();

    int move_cost = attack_speed(*this);

    bool critical_hit = scored_crit(t.dodge_roll());

    // Pick one or more special attacks
    ma_technique technique;
    if (allow_special && force_technique == "") {
        technique = ma_techniques[pick_technique(t, critical_hit, false, false)];
    } else if (allow_special && force_technique != "") {
        technique = ma_techniques[force_technique];
    } else
        technique = ma_techniques["tec_none"];

    int hit_spread = t.deal_melee_attack(this, hit_roll());
    if (hit_spread < 0) {
        int stumble_pen = stumble(*this);
        if (is_player()) { // Only display messages if this is the player
            if (has_miss_recovery_tec())
                add_msg(_("You feint."));
            else if (stumble_pen >= 60)
                add_msg(m_bad, _("You miss and stumble with the momentum."));
            else if (stumble_pen >= 10)
                add_msg(_("You swing wildly and miss."));
            else
                add_msg(_("You miss."));
        }

        if (!has_active_bionic("bio_cqb")) //no practice if you're relying on bio_cqb to fight for you
            melee_practice(calendar::turn, *this, false, unarmed_attack(),
                        weapon.is_bashing_weapon(), weapon.is_cutting_weapon(),
                        (weapon.has_flag("SPEAR") || weapon.has_flag("STAB")));
        move_cost += stumble_pen;
        if (has_miss_recovery_tec())
            move_cost = rng(move_cost / 3, move_cost);
    } else {
        int bash_dam = roll_bash_damage(false);
        int cut_dam  = roll_cut_damage(false);
        int stab_dam = roll_stab_damage(false);

        // multiply damage by style damage_mults
        bash_dam *= mabuff_bash_mult();
        cut_dam *= mabuff_cut_mult();
        stab_dam *= mabuff_cut_mult();

        // Handles effects as well; not done in melee_affect_*
        if (technique.id != "tec_none" && technique.id != "")
            perform_technique(technique, t, bash_dam, cut_dam, stab_dam, move_cost);

        damage_instance d;
        if (critical_hit) // criticals have extra %arpen
            d.add_damage(DT_BASH, bash_dam * 1.5, 0, 0.5);
        else
            d.add_damage(DT_BASH, bash_dam);
        if (cut_dam > stab_dam)
            if (critical_hit) // criticals have extra flat arpen
                d.add_damage(DT_CUT, cut_dam, 5);
            else
                d.add_damage(DT_CUT, cut_dam);
        else {
            if (critical_hit) // stab criticals have extra extra %arpen
                d.add_damage(DT_STAB, stab_dam, 0, 0.33);
            else
                d.add_damage(DT_STAB, stab_dam);
        }

        // Handles speed penalties to monster & us, etc
        std::string specialmsg = melee_special_effects(t, d, technique);

        perform_special_attacks(t);

        dealt_damage_instance dealt_dam; // gets overwritten with the dealt damage values
        t.deal_melee_hit(this, hit_spread, critical_hit, d, dealt_dam);

        // Make a rather quiet sound, to alert any nearby monsters
        if (!is_quiet()) // check martial arts silence
            g->sound(posx, posy, 8, "");

        int dam = dealt_dam.total_damage();

        bool bashing = (d.type_damage(DT_BASH) >= 10 && !unarmed_attack());
        bool cutting = (d.type_damage(DT_CUT) >= 10);
        bool stabbing = (d.type_damage(DT_STAB) >= 10);

        if (!has_active_bionic("bio_cqb")) //no practice if you're relying on bio_cqb to fight for you
            melee_practice(calendar::turn, *this, true, unarmed_attack(), bashing, cutting, stabbing);

        if (dam >= 5 && has_artifact_with(AEP_SAP_LIFE))
            healall( rng(dam / 10, dam / 5) );

        message = melee_message(technique.id, *this, bash_dam, cut_dam, stab_dam);
        player_hit_message(this, message, target_name, dam, critical_hit);

        if (!specialmsg.empty())
            add_msg_if_player(specialmsg.c_str());
    }

    mod_moves(-move_cost);

    ma_onattack_effects(); // trigger martial arts on-attack effects

    return;
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

 int unarmed_skill = skillLevel("unarmed");
 int bashing_skill = skillLevel("bashing");
 int cutting_skill = skillLevel("cutting");
 int stabbing_skill = skillLevel("stabbing");
 int melee_skill = skillLevel("melee");

 if (has_active_bionic("bio_cqb")) {
     unarmed_skill = 5;
     bashing_skill = 5;
     cutting_skill = 5;
     stabbing_skill = 5;
     melee_skill = 5;
 }
// Weapon to-hit roll
 int chance = 25;
 if (unarmed_attack()) { // Unarmed attack: 1/2 of unarmed skill is to-hit
  for (int i = 1; i <= int(unarmed_skill * .5); i++)
   chance += (50 / (2 + i));
 }
 if (weapon.type->m_to_hit > 0) {
  for (int i = 1; i <= weapon.type->m_to_hit; i++)
   chance += (50 / (2 + i));
 } else if (weapon.type->m_to_hit < 0) {
  for (int i = 0; i > weapon.type->m_to_hit; i--)
   chance /= 2;
 }
 if (rng(0, 99) < chance + 4 * disease_intensity("attack_boost") + 4 * mabuff_tohit_bonus())
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

 if (weapon.is_bashing_weapon() && bashing_skill > best_skill)
  best_skill = bashing_skill;
 if (weapon.is_cutting_weapon() && cutting_skill > best_skill)
  best_skill = cutting_skill;
 if ((weapon.has_flag("SPEAR") || weapon.has_flag("STAB")) &&
     stabbing_skill > best_skill)
  best_skill = stabbing_skill;
 if (unarmed_attack() && unarmed_skill > best_skill)
  best_skill = unarmed_skill;

 best_skill += int(melee_skill / 2.5);

 chance = 25;
 if (best_skill > 3) {
  for (int i = 3; i < best_skill; i++)
   chance += (50 / (2 + i));
 } else if (best_skill < 3) {
  for (int i = 3; i > best_skill; i--)
   chance /= 2;
 }
 if (rng(0, 99) < chance + 4 * disease_intensity("attack_boost") + 4 * mabuff_tohit_bonus())
  num_crits++;

 if (num_crits == 3)
  return true;
 else if (num_crits == 2)
  return (hit_roll() >= target_dodge * 1.5 && !one_in(4));

 return false;
}

int player::get_dodge_base() {
    // Creature::get_dodge_base includes stat calculations already
    return Creature::get_dodge_base() + skillLevel("dodge");
}

int player::get_dodge()
//Returns 1/2*DEX + dodge skill level + static bonuses from mutations
//Return numbers range from around 4 (starting player, no boosts) to 29 (20 DEX, 10 dodge, +9 mutations)
{
    //If we're asleep or busy we can't dodge
    if (has_disease("sleep") || has_disease("lying_down")) {return 0;}
    if (activity.type != ACT_NULL) {return 0;}

    return Creature::get_dodge();
}

int player::dodge_roll()
{
    int dodge_stat = get_dodge();

    if (dodges_left <= 0) { // We already dodged this turn
        if (rng(0, skillLevel("dodge") + dex_cur + 15) <= skillLevel("dodge") + dex_cur) {
            dodge_stat = rng(dodge_stat/2, dodge_stat); //Penalize multiple dodges per turn
        } else {
            dodge_stat = 0;
        }
    }

    return dice(dodge_stat, 10); //Matches NPC and monster dodge_roll functions
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

int player::roll_bash_damage(bool crit)
{
    int ret = 0;
    int stat = str_cur; // Which stat determines damage?

    int bashing_skill = skillLevel("bashing");
    int unarmed_skill = skillLevel("unarmed");

    if (has_active_bionic("bio_cqb")) {
        bashing_skill = 5;
        unarmed_skill = 5;
    }

    int skill = bashing_skill; // Which skill determines damage?

    stat += mabuff_bash_bonus();

    if (unarmed_attack())
        skill = unarmed_skill;

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
        bash_dam = rng(0, int(stat / 2) + unarmed_skill);
    else
        // 80%, 88%, 96%, 104%, 112%, 116%, 120%, 124%, 128%, 132%
        if (bashing_skill <= 5)
            ret *= 0.8 + 0.08 * bashing_skill;
        else
            ret *= 0.92 + 0.04 * bashing_skill;

    if (crit) {
        bash_dam *= 1.5;
        bash_cap *= 2;
    }

    if (bash_dam > bash_cap)// Cap for weak characters
        bash_dam = (bash_cap * 3 + bash_dam) / 4;

    /* TODO: handle this in deal_damage
    if (z != NULL && z->has_flag(MF_PLASTIC))
    bash_dam /= rng(2, 4);
    */

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
    }

    return (ret < 0 ? 0 : ret);
}

int player::roll_cut_damage(bool crit)
{
    if (weapon.has_flag("SPEAR"))
        return 0;  // Stabs, doesn't cut!

    double ret = mabuff_cut_bonus() + weapon.damage_cut();

    int cutting_skill = skillLevel("cutting");
    int unarmed_skill = skillLevel("unarmed");

    if (has_active_bionic("bio_cqb"))
    {
        cutting_skill = 5;
        unarmed_skill = 5;
    }

    if (unarmed_attack() && !wearing_something_on(bp_hands)) {
        if (has_trait("CLAWS") || has_trait("CLAWS_RETRACT"))
            ret += 6;
        if (has_bionic("bio_razors"))
            ret += 4;
        if (has_trait("TALONS"))
            ret += 6 + (unarmed_skill > 8 ? 8 : unarmed_skill);
        //TODO: add acidproof check back to slime hands (probably move it elsewhere)
        if (has_trait("SLIME_HANDS"))
            ret += rng(4, 6);
    }

    if (ret <= 0)
        return 0; // No negative damage!


    // 80%, 88%, 96%, 104%, 112%, 116%, 120%, 124%, 128%, 132%
    if (cutting_skill <= 5)
        ret *= 0.8 + 0.08 * cutting_skill;
    else
        ret *= 0.92 + 0.04 * cutting_skill;

    if (crit)
        ret *= 1.0 + (cutting_skill / 12.0);

    return ret;
}

int player::roll_stab_damage(bool crit)
{
    double ret = 0;
    //TODO: armor formula is z->get_armor_cut() - 3 * skillLevel("stabbing")

    if (unarmed_attack() && !wearing_something_on(bp_hands)) {
        ret = 0;
        if (has_trait("CLAWS") || has_trait("CLAWS_RETRACT"))
            ret += 6;
        if (has_trait("NAILS"))
            ret++;
        if (has_bionic("bio_razors"))
            ret += 4;
        if (has_trait("THORNS"))
            ret += 4;
    } else if (weapon.has_flag("SPEAR") || weapon.has_flag("STAB"))
        ret = weapon.damage_cut();
    else
        return 0; // Can't stab at all!

    /* TODO: add this bonus back in
    if (z != NULL && z->speed > 100) { // Bonus against fast monsters
    int speed_min = (z->speed - 100) / 10, speed_max = (z->speed - 100) / 5;
    int speed_dam = rng(speed_min, speed_max);
    if (speed_dam > ret * 2)
    speed_dam = ret * 2;
    if (speed_dam > 0)
    ret += speed_dam;
    }
    */

    if (ret <= 0)
        return 0; // No negative stabbing!

    int stabbing_skill = skillLevel("stabbing");

    if (has_active_bionic("bio_cqb"))
        stabbing_skill = 5;

    // 76%, 86%, 96%, 106%, 116%, 122%, 128%, 134%, 140%, 146%
    if (stabbing_skill <= 5)
        ret *= 0.66 + 0.1 * stabbing_skill;
    else
        ret *= 0.86 + 0.06 * stabbing_skill;

    if (crit)
        ret *= 1.0 + (stabbing_skill / 10.0);

    return ret;
}

// Chance of a weapon sticking is based on weapon attack type.
// Only an issue for cutting and piercing weapons.
// Attack modes are "CHOP", "STAB", and "SLICE".
// "SPEAR" is synonymous with "STAB".
// Weapons can have a "low_stick" flag indicating they
// Have a feature to prevent sticking, such as a spear with a crossbar,
// Or a stabbing blade designed to resist sticking.
int player::roll_stuck_penalty(bool stabbing, ma_technique &tec)
{
    // The cost of the weapon getting stuck, in units of move points.
    const int weapon_speed = attack_speed(*this);
    int stuck_cost = weapon_speed;
    int attack_skill = stabbing ? skillLevel("stabbing") : skillLevel("cutting");

    if (has_active_bionic("bio_cqb"))
        attack_skill = 5;

    const float cut_damage = weapon.damage_cut();
    const float bash_damage = weapon.damage_bash();
    float cut_bash_ratio = 0.0;

    // Scale cost along with the ratio between cutting and bashing damage of the weapon.
    if( cut_damage > 0.0 || bash_damage > 0.0 )
    {
        cut_bash_ratio = cut_damage / ( cut_damage + bash_damage );
    }
    stuck_cost *= cut_bash_ratio;

    if (tec.aoe == "impale") {
        // Impaling attacks should almost always stick
        stuck_cost *= 5.00;
    }
    else if( weapon.has_flag("SLICE") )
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
    // Cap stuck penalty at 2x weapon speed under normal circumstances
    if (tec.aoe != "impale") {
        stuck_cost = std::min( stuck_cost, 2*weapon_speed );
    }

    return stuck_cost;
}

matec_id player::pick_technique(Creature &t,
                                bool crit, bool dodge_counter, bool block_counter)
{

    std::vector<matec_id> all = get_all_techniques();

    std::vector<matec_id> possible;

    bool downed = t.has_effect("downed");

    // first add non-aoe tecs
    for (std::vector<matec_id>::const_iterator it = all.begin();
         it != all.end(); ++it) {
        ma_technique tec = ma_techniques[*it];

        // ignore "dummy" techniques like WBLOCK_1
        if (tec.id.length() == 0) {
            continue;
        }

        // skip defensive techniques
        if (tec.defensive) {
            continue;
        }

        // skip normal techniques if looking for a dodge counter
        if (dodge_counter && !tec.dodge_counter) {
            continue;
        }

        // skip normal techniques if looking for a block counter
        if (block_counter && !tec.block_counter) {
            continue;
        }

        // if crit then select only from crit tecs
        // dodge and blocks roll again for their attack, so ignore crit state
        if (!dodge_counter && !block_counter && ((crit && !tec.crit_tec) || (!crit && tec.crit_tec)) ) {
            continue;
        }

        // don't apply downing techniques to someone who's already downed
        if (downed && tec.down_dur > 0) {
            continue;
        }

        // don't apply disarming techniques to someone without a weapon
        // TODO: these are the stat reqs for tec_disarm
        // dice(   dex_cur +    skillLevel("unarmed"),  8) >
        // dice(p->dex_cur + p->skillLevel("melee"),   10))
        if (tec.disarms && !t.has_weapon()) {
            continue;
        }

        // if aoe, check if there are valid targets
        if (tec.aoe.length() > 0 && !valid_aoe_technique(t, tec)) {
            continue;
        }

        if (tec.is_valid_player(*this)) {
            possible.push_back(tec.id);

            //add weighted options into the list extra times, to increase their chance of being selected
            if (tec.weighting > 1) {
                for (int i = 1; i < tec.weighting; i++) {
                    possible.push_back(tec.id);
                }
            }
        }
    }

    if (possible.empty()) {
        return "tec_none";
    }

    return possible[ rng(0, possible.size() - 1) ];
}

bool player::valid_aoe_technique( Creature &t, ma_technique &technique )
{
    std::vector<int> dummy_mon_targets;
    std::vector<int> dummy_npc_targets;
    return valid_aoe_technique( t, technique, dummy_mon_targets, dummy_npc_targets );
}

bool player::valid_aoe_technique( Creature &t, ma_technique &technique,
                                  std::vector<int> &mon_targets, std::vector<int> &npc_targets )
{
    if (technique.aoe.length() == 0) {
        return false;
    }

    //wide hits all targets adjacent to the attacker and the target
    if( technique.aoe == "wide" ) {
        //check if either (or both) of the squares next to our target contain a possible victim
        //offsets are a pre-computed matrix allowing us to quickly lookup adjacent squares
        int offset_a[] = {0 ,-1,-1,1 ,0 ,-1,1 ,1 ,0 };
        int offset_b[] = {-1,-1,0 ,-1,0 ,1 ,0 ,1 ,1 };

        int lookup = t.ypos() - posy + 1 + (3 * (t.xpos() - posx + 1));

        int left_x = posx + offset_a[lookup];
        int left_y = posy + offset_b[lookup];
        int right_x = posx + offset_b[lookup];
        int right_y = posy - offset_a[lookup];

        int mondex_l = g->mon_at(left_x, left_y);
        int mondex_r = g->mon_at(right_x, right_y);
        if (mondex_l != -1 && g->zombie(mondex_l).friendly == 0) {
            mon_targets.push_back(mondex_l);
        }
        if (mondex_r != -1 && g->zombie(mondex_r).friendly == 0) {
            mon_targets.push_back(mondex_r);
        }

        int npcdex_l = g->npc_at(left_x, left_y);
        int npcdex_r = g->npc_at(right_x, right_y);
        if (npcdex_l != -1 && g->active_npc[npcdex_l]->attitude == NPCATT_KILL) {
            npc_targets.push_back(npcdex_l);
        }
        if (npcdex_r != -1 && g->active_npc[npcdex_r]->attitude == NPCATT_KILL) {
            npc_targets.push_back(npcdex_r);
        }
        if(!(npc_targets.empty() && mon_targets.empty())) {
            return true;
        }
    }

    if( technique.aoe == "impale" ) {
        // Impale hits the target and a single target behind them
        // Check if the square cardinally behind our target, or to the left / right,
        // contains a possible target.
        // Offsets are a pre-computed matrix allowing us to quickly lookup adjacent squares.
        int offset_a[] = {0 ,-1,-1,1 ,0 ,-1,1 ,1 ,0 };
        int offset_b[] = {-1,-1,0 ,-1,0 ,1 ,0 ,1 ,1 };

        int lookup = t.ypos() - posy + 1 + (3 * (t.xpos() - posx + 1));

        int left_x = t.xpos() + offset_a[lookup];
        int left_y = t.ypos() + offset_b[lookup];
        int target_x = t.xpos() + (t.xpos() - posx);
        int target_y = t.ypos() + (t.ypos() - posy);
        int right_x = t.xpos() + offset_b[lookup];
        int right_y = t.ypos() - offset_a[lookup];

        int mondex_l = g->mon_at(left_x, left_y);
        int mondex_t = g->mon_at(target_x, target_y);
        int mondex_r = g->mon_at(right_x, right_y);
        if (mondex_l != -1 && g->zombie(mondex_l).friendly == 0) {
            mon_targets.push_back(mondex_l);
        }
        if (mondex_t != -1 && g->zombie(mondex_t).friendly == 0) {
            mon_targets.push_back(mondex_t);
        }
        if (mondex_r != -1 && g->zombie(mondex_r).friendly == 0) {
            mon_targets.push_back(mondex_r);
        }

        int npcdex_l = g->npc_at(left_x, left_y);
        int npcdex_t = g->npc_at(target_x, target_y);
        int npcdex_r = g->npc_at(right_x, right_y);
        if (npcdex_l != -1 && g->active_npc[npcdex_l]->attitude == NPCATT_KILL) {
            npc_targets.push_back(npcdex_l);
        }
        if (npcdex_t != -1 && g->active_npc[npcdex_t]->attitude == NPCATT_KILL) {
            npc_targets.push_back(npcdex_t);
        }
        if (npcdex_r != -1 && g->active_npc[npcdex_r]->attitude == NPCATT_KILL) {
            npc_targets.push_back(npcdex_r);
        }
        if(!(npc_targets.empty() && mon_targets.empty())) {
            return true;
        }
    }

    if( npc_targets.empty() && mon_targets.empty() && technique.aoe == "spin" ) {
        for (int x = posx - 1; x <= posx + 1; x++) {
            for (int y = posy - 1; y <= posy + 1; y++) {
                if (x == t.xpos() && y == t.ypos()) {
                    continue;
                }
                int mondex = g->mon_at(x, y);
                if (mondex != -1 && g->zombie(mondex).friendly == 0) {
                    mon_targets.push_back(mondex);
                }
                int npcdex = g->npc_at(x, y);
                if (npcdex != -1 && g->active_npc[npcdex]->attitude == NPCATT_KILL) {
                    npc_targets.push_back(npcdex);
                }
            }
        }
        //don't trigger circle for fewer than 4 targets
        if( npc_targets.size() + mon_targets.size() < 4 ) {
            npc_targets.clear();
            mon_targets.clear();
        } else {
            return true;
        }
    }
    return false;
}

bool player::has_technique(matec_id id) {
  return weapon.has_technique(id) ||
    martialarts[style_selected].has_technique(*this, id);
}

void player::perform_technique(ma_technique technique, Creature &t, int &bash_dam, int &cut_dam,
                               int &stab_dam, int &move_cost)
{
    std::string target = t.disp_name();

    bash_dam += technique.bash;
    if (cut_dam > stab_dam) { // cut affects stab damage too since only one of cut/stab is used
        cut_dam += technique.cut;
    } else {
        stab_dam += technique.cut;
    }

    bash_dam *= technique.bash_mult;
    cut_dam *= technique.cut_mult;
    stab_dam *= technique.cut_mult;

    move_cost *= technique.speed_mult;

    int tarx = t.xpos(), tary = t.ypos();

    (void) tarx;
    (void) tary;

    if (technique.down_dur > 0) {
        if (t.get_throw_resist() == 0) {
            t.add_effect("downed", rng(1, technique.down_dur));
            bash_dam += 3;
        }
    }

    if (technique.stun_dur > 0) {
        t.add_effect("stunned", rng(1, technique.stun_dur));
    }

    if (technique.knockback_dist > 0) {
        int kb_offset = rng(
                            -technique.knockback_spread,
                            technique.knockback_spread
                        );
        t.knock_back_from(posx + kb_offset, posy + kb_offset);
    }

    if (technique.pain > 0) {
        t.pain += rng(technique.pain / 2, technique.pain);
    }

    /* TODO: put all this in when disease/effects merging is done
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
    */

    //AOE attacks, feel free to skip over this lump
    if (technique.aoe.length() > 0) {
        int temp_moves = moves;

        std::vector<int> mon_targets = std::vector<int>();
        std::vector<int> npc_targets = std::vector<int>();

        valid_aoe_technique( t, technique, mon_targets, npc_targets );

        //hit only one valid target (pierce through doesn't spread out)
        if (technique.aoe == "impale") {
            int victim = rng(0, mon_targets.size() + npc_targets.size() - 1);
            if (victim > mon_targets.size()) {
                victim -= mon_targets.size();
                mon_targets.clear();
                int npc_id = npc_targets[victim];
                npc_targets.clear();
                npc_targets.push_back(npc_id);
            } else {
                npc_targets.clear();
                int mon_id = mon_targets[victim];
                mon_targets.clear();
                mon_targets.push_back(mon_id);
            }
        }

        //hit the targets in the lists (all candidates if wide or burst, or just the unlucky sod if deep)
        int count_hit = 0;
        for (int i = 0; i < mon_targets.size(); i++) {
            if (hit_roll() >= rng(0, 5) + g->zombie(mon_targets[i]).dodge_roll()) {
                count_hit++;
                melee_attack(g->zombie(mon_targets[i]), false);

                std::string temp_target = string_format(_("the %s"), g->zombie(mon_targets[i]).name().c_str());
                add_msg_player_or_npc( m_good, _("You hit %s!"), _("<npcname> hits %s!"), temp_target.c_str() );
            }
        }
        for (int i = 0; i < npc_targets.size(); i++) {
            if (hit_roll() >= rng(0, 5) + g->active_npc[npc_targets[i]]->dodge_roll()) {
                count_hit++;
                melee_attack(*g->active_npc[npc_targets[i]], false);

                add_msg_player_or_npc( m_good, _("You hit %s!"), _("<npcname> hits %s!"),
                                          g->active_npc[npc_targets[i]]->name.c_str() );
            }
        }

        t.add_msg_if_player(m_good, ngettext("%d enemy hit!", "%d enemies hit!", count_hit), count_hit);

        //AOE attacks should take longer than normal, but less than individual attacks.
        moves = temp_moves - ((temp_moves - moves) / 4);
    }

    //player has a very small chance, based on their intelligence, to learn a style whilst using the cqb bionic
    if (has_active_bionic("bio_cqb") && !has_martialart(style_selected)) {
        if (one_in(1400 - (get_int() * 50))) {
            ma_styles.push_back(style_selected);
            add_msg_if_player(m_good, _("You have learnt %s from extensive practice with the CQB Bionic."),
                       martialarts[style_selected].name.c_str());
        }
    }
}

// this would be i2amroy's fix, but it's kinda handy
bool player::can_weapon_block()
{
    return (weapon.has_technique("WBLOCK_1") ||
            weapon.has_technique("WBLOCK_2") ||
            weapon.has_technique("WBLOCK_3"));
}

void player::dodge_hit(Creature *source, int) {
    if (dodges_left < 1)
        return;

    ma_ondodge_effects(); // fire martial arts block-triggered effects

    dodges_left--;

    // check if we have any dodge counters
    matec_id tec = pick_technique(*source, false, true, false);

    if (tec != "tec_none") {
        melee_attack(*source, true, tec);
    }
}

bool player::block_hit(Creature *source, body_part &bp_hit, int &side,
                       damage_instance &dam) {

    //Shouldn't block if player is asleep; this only seems to be used by player.
    //g->u.has_disease("sleep") would work as well from looking at other block functions.

    if (blocks_left < 1 || this->has_disease("sleep"))
        return false;

    ma_ongethit_effects(); // fire martial arts on-getting-hit-triggered effects
    // these fire even if the attack is blocked (you still got hit)

    float total_phys_block = mabuff_block_bonus();
    bool conductive_weapon = weapon.conductive();

    //weapon blocks are prefered to arm blocks
    if (can_weapon_block()) {
        add_msg_player_or_npc( _("You block with your %s!"),
            _("<npcname> blocks with their %s!"), weapon.tname().c_str() );
        handle_melee_wear();
    }
    else if (can_limb_block()) {
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

        add_msg_player_or_npc( _("You block with your %s!"),
            _("<npcname> blocks with their %s!"),
            body_part_name(bp_hit, side).c_str());
    }

    float phys_mult = 1.0f;
    float block_amount;
    for (std::vector<damage_unit>::iterator it = dam.damage_units.begin();
            it != dam.damage_units.end(); ++it) {
        // block physical damage "normally"
        if (it->type == DT_BASH || it->type == DT_CUT || it->type == DT_STAB) {
            // use up our flat block bonus first
            block_amount = std::min(total_phys_block, it->amount);
            total_phys_block -= block_amount;
            it->amount -= block_amount;

            if (can_weapon_block()) {
                if (weapon.has_technique("WBLOCK_1")) {
                    phys_mult = 0.4;
                } else if (weapon.has_technique("WBLOCK_2")) {
                    phys_mult = 0.15;
                } else if (weapon.has_technique("WBLOCK_3")) {
                    phys_mult = 0.05;
                } else {
                    phys_mult = 0.5; // always at least as good as unarmed
                }
            }
            else if (can_limb_block()) {
                phys_mult = 0.5;
            }
            it->amount *= phys_mult;
        // non-electrical "elemental" damage types do their full damage if unarmed,
        // but severely mitigated damage if not
        } else if (it->type == DT_HEAT || it->type == DT_ACID || it->type == DT_COLD) {
            //TODO: should damage weapons if blocked
            if (!unarmed_attack() && can_weapon_block()) {
                it->amount /= 5;
            }
        // electrical damage deals full damage if unarmed OR wielding a
        // conductive weapon
        } else if (it->type == DT_ELECTRIC) {
            if (!unarmed_attack() && can_weapon_block() && !conductive_weapon) {
                it->amount /= 5;
            }
        }
    }

    blocks_left--;

    ma_onblock_effects(); // fire martial arts block-triggered effects

    // check if we have any dodge counters
    matec_id tec = pick_technique(*source, false, false, true);

    if (tec != "tec_none") {
        melee_attack(*source, true, tec);
    }

    return true;
}

void player::perform_special_attacks(Creature &t)
{
 bool can_poison = false;

 std::vector<special_attack> special_attacks = mutation_attacks(t);

 std::string target = t.disp_name();

 for (size_t i = 0; i < special_attacks.size(); i++) {
  dealt_damage_instance dealt_dam;

  int hit_spread = t.deal_melee_attack(this, hit_roll() * 0.8);
  if (hit_spread >= 0)
      t.deal_melee_hit(this, hit_spread, false, damage_instance::physical(
            special_attacks[i].bash,
            special_attacks[i].cut,
            special_attacks[i].stab
        ), dealt_dam);
  if (dealt_dam.total_damage() > 0)
      add_msg_if_player(m_good, special_attacks[i].text.c_str());

  if (!can_poison && (dealt_dam.type_damage(DT_CUT) > 0 ||
        dealt_dam.type_damage(DT_STAB) > 0 ))
   can_poison = true;
 }

 if (can_poison && ((has_trait("POISONOUS")) || (has_trait("POISONOUS2")))) {
    if ((has_trait("POISONOUS")) && !t.has_effect("poisoned")) {
        t.add_msg_if_player(m_good, _("You poison %s!"), target.c_str());
        t.add_effect("poisoned", 6);
    }
    else if ((has_trait("POISONOUS2")) && (!(t.has_effect("nasty_poisoned")))) {
        t.add_msg_if_player(m_good, _("You inject your venom into %s!"), target.c_str());
        t.add_effect("nasty_poisoned", 6);
    }
 }
}

std::string player::melee_special_effects(Creature &t, damage_instance &d, ma_technique &tec)
{
    std::stringstream dump;

    std::string target;

    target = t.disp_name();

    int tarposx = t.xpos(), tarposy = t.ypos();

    (void)tarposx;
    (void)tarposy;

    // Bonus attacks!
    bool shock_them = (has_active_bionic("bio_shock") && power_level >= 2 &&
                       (unarmed_attack() || weapon.made_of("iron") ||
                        weapon.made_of("steel") || weapon.made_of("silver") ||
                        weapon.made_of("gold") || weapon.made_of("superalloy")) && one_in(3));

    bool drain_them = (has_active_bionic("bio_heat_absorb") && power_level >= 1 &&
                       !is_armed() && t.is_warm());
    drain_them &= one_in(2); // Only works half the time

    bool burn_them = weapon.has_flag("FLAMING");


    if (shock_them) { // bionics only
        power_level -= 2;
        int shock = rng(2, 5);
        d.add_damage(DT_ELECTRIC, shock * rng(1, 3));

        if (is_player()) {
            dump << string_format(_("You shock %s."), target.c_str()) << std::endl;
        } else
            add_msg_player_or_npc(m_good, _("You shock %s."),
                                          _("<npcname> shocks %s."),
                                          target.c_str());
    }

    if (drain_them) { // bionics only
        power_level--;
        charge_power(rng(0, 2));
        d.add_damage(DT_COLD, 1);
        if (t.is_player()) {
            add_msg_if_npc(m_bad, _("<npcname> drains your body heat!"));
        } else {
            if (is_player()) {
                dump << string_format(_("You drain %s's body heat."), target.c_str()) << std::endl;
            } else
                add_msg_player_or_npc(m_good, _("You drain %s's body heat!"),
                                              _("<npcname> drains %s's body heat!"),
                                              target.c_str());
        }
    }

    if (burn_them) { // for flaming weapons
        d.add_damage(DT_HEAT, rng(1, 8));

        if (is_player()) {
            dump << string_format(_("You burn %s."), target.c_str()) << std::endl;
        } else
            add_msg_player_or_npc(m_good, _("You burn %s."),
                                     _("<npcname> burns %s."),
                                     target.c_str());
    }

    //Hurting the wielder from poorly-chosen weapons
    if(weapon.has_flag("HURT_WHEN_WIELDED") && x_in_y(2, 3)) {
        add_msg_if_player(m_bad, _("The %s cuts your hand!"), weapon.tname().c_str());
        deal_damage(NULL, bp_hands, 0, damage_instance::physical(0, weapon.damage_cut(), 0));
        if (weapon.is_two_handed(this)) { // Hurt left hand too, if it was big
            deal_damage(NULL, bp_hands, 1, damage_instance::physical(0, weapon.damage_cut(), 0));
        }
    }

    // Glass weapons shatter sometimes
    if (weapon.made_of("glass") &&
        rng(0, weapon.volume() + 8) < weapon.volume() + str_cur) {
        if (is_player()) {
            dump << string_format(_("Your %s shatters!"), weapon.tname().c_str()) << std::endl;
        } else {
            add_msg_player_or_npc(m_bad, _("Your %s shatters!"),
                                     _("<npcname>'s %s shatters!"),
                                     weapon.tname().c_str());
        }

        g->sound(posx, posy, 16, "");
        // Dump its contents on the ground
        for (size_t i = 0; i < weapon.contents.size(); i++) {
            g->m.add_item_or_charges(posx, posy, weapon.contents[i]);
        }
        // Take damage
        deal_damage(this, bp_arms, 1, damage_instance::physical(0, rng(0, weapon.volume() * 2), 0));
        if (weapon.is_two_handed(this)) {// Hurt left arm too, if it was big
            //redeclare shatter_dam because deal_damage mutates it
            deal_damage(this, bp_arms, 0, damage_instance::physical(0, rng(0, weapon.volume() * 2), 0));
        }
        d.add_damage(DT_CUT, rng(0, 5 + int(weapon.volume() * 1.5)));// Hurt the monster extra
        remove_weapon();
    }

    handle_melee_wear();

    bool is_hallucination = false; //Check if the target is an hallucination.
    if(monster *m = dynamic_cast<monster *>(&t)) { //Cast fails if the t is an NPC or the player.
        is_hallucination = m->is_hallucination();
    }

    int cutting_penalty = roll_stuck_penalty(d.type_damage(DT_STAB) > d.type_damage(DT_CUT), tec);
    // Some weapons splatter a lot of blood around.
    // TODO: this function shows total damage done by this attack, not final damage inflicted.
    if (d.total_damage() > 10) { //Check if you do non minor damage
        if (weapon.has_flag("MESSY")) { // e.g. chainsaws
            cutting_penalty /= 6; // Harder to get stuck
            //Get blood type.
            field_id type_blood = fd_blood;
            if(!is_hallucination || t.has_flag(MF_VERMIN)) {
                type_blood = t.bloodType();
            } else {
                type_blood = fd_null;
            }
            if(type_blood != fd_null) {
                for (int x = tarposx - 1; x <= tarposx + 1; x++) {
                    for (int y = tarposy - 1; y <= tarposy + 1; y++) {
                        if (!one_in(3)) {
                            g->m.add_field(x, y, type_blood, 1);
                        }
                    }
                } //it all
            } //comes
        } //tumbling down
    }
    // Getting your weapon stuck
    if (!unarmed_attack() && cutting_penalty > dice(str_cur * 2, 20) && !is_hallucination) {
        dump << string_format(_("Your %s gets stuck in %s, pulling it out of your hands!"),
                              weapon.tname().c_str(), target.c_str());
        // TODO: better speed debuffs for target, possibly through effects
        if (monster *m = dynamic_cast<monster *>(&t)) {
            m->add_item(remove_weapon());
        } else {
            // Happens if 't' is not of 'monster' origin (this shouldn't happen)
            g->m.add_item_or_charges(tarposx, tarposy, remove_weapon(), 1);
        }
        t.mod_moves(-30);
        if (weapon.has_flag("HURT_WHEN_PULLED") && one_in(3)) {
            //Sharp objects that injure wielder when pulled from hands (so cutting damage only)
            dump << std::endl << string_format(_("You are hurt by the %s being pulled from your hands!"),
                                               weapon.tname().c_str());
            deal_damage( this, bp_hands, random_side(bp_hands),
                         damage_instance::physical( 0, weapon.damage_cut() / 2, 0) );
        }
    } else {
        if (d.total_damage() > 20) { // TODO: change this back to "if it would kill the monster"
            cutting_penalty /= 2;
            int cutting_skill = has_active_bionic("bio_cqb") ? 5 : (int)skillLevel("cutting");
            cutting_penalty -= rng(cutting_skill, cutting_skill * 2 + 2);
        }
        if (cutting_penalty >= 50 && !is_hallucination) {
            dump << string_format(_("Your %s gets stuck in %s but you yank it free!"), weapon.tname().c_str(),
                                  target.c_str());
        }
        if (weapon.has_flag("SPEAR") || weapon.has_flag("STAB")) {
            t.mod_moves(-30);
        }
    }
    if (cutting_penalty > 0) {
        mod_moves(-cutting_penalty);
    }

    // on-hit effects for martial arts
    ma_onhit_effects();

    return dump.str();
}

std::vector<special_attack> player::mutation_attacks(Creature &t)
{
    std::vector<special_attack> ret;

    std::string target = t.disp_name();

    if ( (has_trait("SABER_TEETH")) && !wearing_something_on(bp_mouth) &&
         one_in(20 - dex_cur - skillLevel("unarmed")) ) {
        special_attack tmp;
        tmp.stab = (25 + str_cur);
        if (is_player()) {
            tmp.text = string_format(_("You tear into %s with your saber teeth!"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%s tears into %s with his saber teeth!"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%s tears into %s with her saber teeth!"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    //Having lupine or croc jaws makes it much easier to sink your fangs into people; Ursine/Feline, not so much
    if (has_trait("FANGS") && (!wearing_something_on(bp_mouth)) &&
        ((!has_trait("MUZZLE") && !has_trait("MUZZLE_LONG") &&
          one_in(20 - dex_cur - skillLevel("unarmed"))) ||
         (has_trait("MUZZLE") && one_in(18 - dex_cur - skillLevel("unarmed"))) ||
         (has_trait("MUZZLE_LONG") && one_in(15 - dex_cur - skillLevel("unarmed"))))) {
        special_attack tmp;
        tmp.stab = 20;
        if (is_player()) {
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
        if (is_player()) {
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

    if (!has_trait("FANGS") && has_trait("MUZZLE_BEAR") &&
        one_in(20 - dex_cur - skillLevel("unarmed")) &&
        (!wearing_something_on(bp_mouth))) {
        special_attack tmp;
        tmp.cut = 5;
        if (is_player()) {
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

    if (!has_trait("FANGS") && has_trait("MUZZLE_LONG") &&
        one_in(18 - dex_cur - skillLevel("unarmed")) &&
        (!wearing_something_on(bp_mouth))) {
        special_attack tmp;
        tmp.stab = 18;
        if (is_player()) {
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
        if (is_player()) {
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
        if (is_player()) {
            tmp.text = string_format(_("You peck %s!"),
                                     target.c_str());
        } else {
            tmp.text = string_format(_("%s pecks %s!"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    if (has_trait("BEAK_PECK") && one_in(15 - dex_cur - skillLevel("unarmed")) &&
        (!wearing_something_on(bp_mouth))) {
        // method open to improvement, please feel free to suggest
        // a better way to simulate target's anti-peck efforts
        int num_hits = (dex_cur + skillLevel("unarmed") - rng(4, 10));
        if (num_hits <= 0) {
            num_hits = 1;
        }
        // Yeah, arbitrary balance cap of Unfunness. :-(
        // Though this is a 6-second turn, so only so much
        // time to peck your target.
        if (num_hits >= 5) {
            num_hits = 5;
        }
        special_attack tmp;
        tmp.stab = (num_hits *= 10 );
        if (num_hits == 1) {
            if (is_player()) {
                tmp.text = string_format(_("You peck %s!"),
                                         target.c_str());
            } else {
                tmp.text = string_format(_("%s pecks %s!"),
                                         name.c_str(), target.c_str());
            }
        }
        //~"jackhammering" with the beak is metaphor for the rapid-peck
        //~commonly employed by a woodpecker drilling into wood
        else {
            if (is_player()) {
                tmp.text = string_format(_("You jackhammer into %s with your beak!"),
                                         target.c_str());
            } else if (male) {
                tmp.text = string_format(_("%s jackhammers into %s with his beak!"),
                                         name.c_str(), target.c_str());
            } else {
                tmp.text = string_format(_("%s jackhammers into %s with her beak!"),
                                         name.c_str(), target.c_str());
            }
        }
        ret.push_back(tmp);
    }

    if (has_trait("HOOVES") && one_in(25 - dex_cur - 2 * skillLevel("unarmed"))) {
        special_attack tmp;
        tmp.bash = str_cur * 3;
        if (tmp.bash > 40) {
            tmp.bash = 40;
        }
        if (is_player()) {
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
        if (is_player()) {
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
        if (is_player()) {
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
        if (is_player()) {
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

    if (has_trait("HORNS_POINTED") && one_in(22 - dex_cur - skillLevel("unarmed"))) {
        special_attack tmp;
        tmp.stab = 24;
        if (is_player()) {
            tmp.text = string_format(_("You stab %s with your pointed horns!"),
                                     target.c_str());
        } else {
            tmp.text = string_format(_("%s stabs %s with their pointed horns!"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    if (has_trait("ANTLERS") && one_in(20 - dex_cur - skillLevel("unarmed"))) {
        special_attack tmp;
        tmp.bash = 4;
        if (is_player()) {
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
        if (is_player()) {
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
        if (is_player()) {
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
        if (is_player()) {
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
            special_attack tmp;
            // Tentacle Rakes add additional cutting damage
            if (is_player()) {
                if (has_trait("CLAWS_TENTACLE")) {
                    tmp.text = string_format(_("You rake %s with your tentacle!"),
                                             target.c_str());
                } else tmp.text = string_format(_("You slap %s with your tentacle!"),
                                                    target.c_str());
            } else if (male) {
                if (has_trait("CLAWS_TENTACLE")) {
                    tmp.text = string_format(_("%s rakes %s with his tentacle!"),
                                             name.c_str(), target.c_str());
                } else tmp.text = string_format(_("%s slaps %s with his tentacle!"),
                                                    name.c_str(), target.c_str());
            } else {
                if (has_trait("CLAWS_TENTACLE")) {
                    tmp.text = string_format(_("%s rakes %s with her tentacle!"),
                                             name.c_str(), target.c_str());
                } else tmp.text = string_format(_("%s slaps %s with her tentacle!"),
                                                    name.c_str(), target.c_str());
            }
            if (has_trait("CLAWS_TENTACLE")) {
                tmp.cut = str_cur / 2 + 1;
            } else {
                tmp.bash = str_cur / 3 + 1;
            }
            ret.push_back(tmp);
        }
    }

    if (has_trait("VINES2") || has_trait("VINES3")) {
        int num_attacks = 2;
        if (has_trait("VINES3")) {
            num_attacks = 3;
        }
        for (int i = 0; i < num_attacks; i++) {
            special_attack tmp;
            if (is_player()) {
                tmp.text = string_format(_("You lash %s with a vine!"),
                                         target.c_str());
            } else if (male) {
                tmp.text = string_format(_("%s lashes %s with his vines!"),
                                         name.c_str(), target.c_str());
            } else {
                tmp.text = string_format(_("%s lashes %s with her vines!"),
                                         name.c_str(), target.c_str());
            }
            tmp.bash = str_cur / 2;
            ret.push_back(tmp);
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
             return npc ? _("<npcname> runs %s through") : _("You run %s through");
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
void player_hit_message(player* attacker, std::string message,
                        std::string target_name, int dam, bool crit)
{
    std::string msg;
    game_message_type msgtype;
    msgtype = m_good;
    if (dam <= 0) {
        if (attacker->is_npc()) {
            //~ NPC hits something but does no damage
            msg = string_format(_("%s but does no damage."), message.c_str());
        } else {
            //~ you hit something but do no damage
            msg = string_format(_("%s but do no damage."), message.c_str());
        }
        msgtype = m_neutral;
    } else if (crit) {
        //~ someone hits something for %d damage (critical)
        msg = string_format(_("%s for %d damage. Critical!"),
                            message.c_str(), dam);
        if (!attacker->is_npc()) {
            msgtype = m_good;
        }
    } else {
        //~ someone hits something for %d damage
        msg = string_format(_("%s for %d damage."), message.c_str(), dam);
        if (!attacker->is_npc()) {
            msgtype = m_good;
        }
    }

    // same message is used for player and npc,
    // just using this for the <npcname> substitution.
    attacker->add_msg_player_or_npc(msgtype, msg.c_str(), msg.c_str(),
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
 int melee_skill = u.has_active_bionic("bio_cqb") ? 5 : (int)u.skillLevel("melee");
 int skill_cost = (int)(move_cost / (std::pow(melee_skill, 3.0f)/400.0 + 1.0));
 int dexbonus = (int)( std::pow(std::max(u.dex_cur - 8, 0), 0.8) * 3 );

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
