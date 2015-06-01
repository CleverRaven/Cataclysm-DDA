#include "player.h"
#include "bionics.h"
#include "debug.h"
#include "game.h"
#include "map.h"
#include "debug.h"
#include "rng.h"
#include "martialarts.h"
#include "messages.h"
#include "sounds.h"
#include "translations.h"
#include "monster.h"
#include "npc.h"
#include "itype.h"
#include "line.h"

#include <sstream>
#include <stdlib.h>
#include <algorithm>

#include "cursesdef.h"

static const matec_id tec_none( "tec_none" );
static const matec_id WBLOCK_1( "WBLOCK_1" );
static const matec_id WBLOCK_2( "WBLOCK_2" );
static const matec_id WBLOCK_3( "WBLOCK_3" );

void player_hit_message(player* attacker, std::string message,
                        Creature &t, int dam, bool crit);
void melee_practice( player &u, bool hit, bool unarmed, bool bashing, bool cutting, bool stabbing);
int  attack_speed(player &u);
int  stumble(player &u);
std::string melee_message( const ma_technique &tech, player &p, const dealt_damage_instance &ddi );

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

bool player::is_armed() const
{
    return (weapon.typeId() != "null");
}

bool player::handle_melee_wear()
{
    // Here is where we handle wear and tear on things we use as melee weapons or shields.
    int material_factor = 1;
    int damage_chance = dex_cur + ( 2 * get_skill_level("melee") ) + ( 128 / std::max(str_cur,1) );
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
            add_msg_player_or_npc( m_bad, _("Your %s is damaged by the force of the blow!"),
                                    _("<npcname>'s %s is damaged by the force of the blow!"),
                                    weapon.tname().c_str());
            //Don't increment until after the message is displayed
            weapon.damage++;
        } else if (weapon.damage >= 4 && one_in(damage_chance) && (!weapon.has_flag("UNBREAKABLE_MELEE"))) {
            add_msg_player_or_npc( m_bad, _("Your %s is destroyed by the blow!"),
                                    _("<npcname>'s %s is destroyed by the blow!"),
                                    weapon.tname().c_str());
            // Dump its contents on the ground
            for( auto &elem : weapon.contents ) {
                g->m.add_item_or_charges( pos3(), elem );
            }
            remove_weapon();
        }
    }
    return true;
}

bool player::unarmed_attack() const {
 return (weapon.typeId() == "null" || weapon.has_flag("UNARMED_WEAPON"));
}

// TODO: this is here for newcharacter.cpp only, swap it out when possible
int player::base_to_hit(bool real_life, int stat)
{
 if (stat == -999)
  stat = (real_life ? dex_cur : dex_max);
 return 1 + int(stat / 2) + get_skill_level("melee");
}


int player::get_hit_base() const
{
    int best_bonus = 0;

    int unarmed_skill = get_skill_level("unarmed");
    int bashing_skill = get_skill_level("bashing");
    int cutting_skill = get_skill_level("cutting");
    int stabbing_skill = get_skill_level("stabbing");
    int melee_skill = get_skill_level("melee");

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

int player::hit_roll() const
{
    //Unstable ground chance of failure
    if (has_effect("bouldering")) {
        if(one_in(get_dex())) {
            add_msg_if_player(m_bad, _("The ground shifts beneath your feet!"));
            return 0;
        }
    }

    // apply martial arts bonuses
    int numdice = get_hit();

    int sides = 10 - (encumb(bp_torso) / 10);
    int best_bonus = 0;
    if (sides < 2) {
        sides = 2;
    }

    numdice += best_bonus; // Use whichever bonus is best.

    // Drunken master makes us hit better
    if (has_trait("DRUNKEN")) {
        if (unarmed_attack()) {
            numdice += int(get_effect_dur("drunk") / 300);
        } else {
            numdice += int(get_effect_dur("drunk") / 400);
        }
    }

    // Farsightedness makes us hit worse
    if (has_trait("HYPEROPIC") && !is_wearing("glasses_reading")
          && !is_wearing("glasses_bifocal")) {
        numdice -= 2;
    }

    if (numdice < 1) {
        numdice = 1;
        sides = 8 - (encumb(bp_torso) / 10);
    }

    return dice(numdice, sides);
}

void player::add_miss_reason(const char *reason, unsigned int weight)
{
    melee_miss_reasons.add(reason, weight);

}

void player::clear_miss_reasons()
{
    melee_miss_reasons.clear();
}

const char *player::get_miss_reason()
{
    // everything that lowers accuracy in player::hit_roll()
    // adding it in hit_roll() might not be safe if it's called multiple times
    // in one turn
    add_miss_reason(
        _("Your torso encumbrance throws you off-balance."),
        encumb(bp_torso));
    int farsightedness = 2 * (has_trait("HYPEROPIC")
                              && !is_wearing("glasses_reading")
                              && !is_wearing("glasses_bifocal"));
    add_miss_reason(
        _("You can't hit reliably without your glasses."),
        farsightedness);

    const char** reason = melee_miss_reasons.pick();
    if(reason == NULL) {
        return NULL;
    }
    return *reason;
}

// Melee calculation is in parts. This sets up the attack, then in deal_melee_attack,
// we calculate if we would hit. In Creature::deal_melee_hit, we calculate if the target dodges.
void player::melee_attack(Creature &t, bool allow_special, const matec_id &force_technique) {
    const bool has_force_technique = !force_technique.str().empty();
    bool is_u = (this == &(g->u)); // Affects how we'll display messages
    if (!t.is_player()) {
        // @todo Per-NPC tracking? Right now monster hit by either npc or player will draw aggro...
        t.add_effect("hit_by_player", 100); // Flag as attacked by us for AI
    }

    std::string message = is_u ? _("You hit %s") : _("<npcname> hits %s");

    int move_cost = attack_speed(*this);

    bool critical_hit = scored_crit(t.dodge_roll());

    // Pick one or more special attacks
    matec_id technique_id;
    if( allow_special && !has_force_technique ) {
        technique_id = pick_technique(t, critical_hit, false, false);
    } else if (allow_special && has_force_technique) {
        technique_id = force_technique;
    } else {
        technique_id = tec_none;
    }
    const ma_technique &technique = technique_id.obj();
    int hit_spread = t.deal_melee_attack(this, hit_roll());
    if (hit_spread < 0) {
        int stumble_pen = stumble(*this);
        if (is_player()) { // Only display messages if this is the player

            if (one_in(2)) {
                const char* reason_for_miss = get_miss_reason();
                if (reason_for_miss != NULL)
                    add_msg(reason_for_miss);
            }

            if (has_miss_recovery_tec())
                add_msg(_("You feint."));
            else if (stumble_pen >= 60)
                add_msg(m_bad, _("You miss and stumble with the momentum."));
            else if (stumble_pen >= 10)
                add_msg(_("You swing wildly and miss."));
            else
                add_msg(_("You miss."));
        } else if( g->u.sees( *this ) ) {
            if (stumble_pen >= 60)
                add_msg( _("%s misses and stumbles with the momentum."),name.c_str());
            else if (stumble_pen >= 10)
                add_msg(_("%s swings wildly and misses."),name.c_str());
            else
                add_msg(_("%s misses."),name.c_str());
        }
        if (!has_active_bionic("bio_cqb")) //no practice if you're relying on bio_cqb to fight for you
            melee_practice( *this, false, unarmed_attack(),
                            weapon.is_bashing_weapon(), weapon.is_cutting_weapon(),
                            (weapon.has_flag("SPEAR") || weapon.has_flag("STAB")));
        move_cost += stumble_pen;
        if (has_miss_recovery_tec()) {
            move_cost = rng(move_cost / 3, move_cost);
        }
    } else {
        damage_instance d;
        roll_bash_damage( critical_hit, d );
        roll_cut_damage ( critical_hit, d );
        roll_stab_damage( critical_hit, d );

        // Handles effects as well; not done in melee_affect_*
        if( technique.id != tec_none ) {
            perform_technique(technique, t, d, move_cost);
        }

        // Handles speed penalties to monster & us, etc
        std::string specialmsg = melee_special_effects(t, d, technique);

        perform_special_attacks(t);

        dealt_damage_instance dealt_dam; // gets overwritten with the dealt damage values
        t.deal_melee_hit(this, hit_spread, critical_hit, d, dealt_dam);

        // Make a rather quiet sound, to alert any nearby monsters
        if (!is_quiet()) { // check martial arts silence
            sounds::sound( pos3(), 8, "", false, "melee_hit", weapon.typeId() );
        }

        int dam = dealt_dam.total_damage();

        bool bashing = (d.type_damage(DT_BASH) >= 10 && !unarmed_attack());
        bool cutting = (d.type_damage(DT_CUT) >= 10);
        bool stabbing = (d.type_damage(DT_STAB) >= 10);

        // Set the highest damage type to true.
        if( !unarmed_attack() ) {
            if( d.type_damage(DT_BASH) > d.type_damage(DT_CUT) ) {
                if( d.type_damage(DT_BASH) > d.type_damage(DT_STAB) ) {
                    bashing = true;
                } else {
                    stabbing = true;
                }
            } else {
                if( d.type_damage(DT_CUT) > d.type_damage(DT_STAB) ) {
                    cutting = true;
                } else {
                    stabbing = true;
                }
            }
        }

        if (!has_active_bionic("bio_cqb")) {
            //no practice if you're relying on bio_cqb to fight for you
            melee_practice( *this, true, unarmed_attack(), bashing, cutting, stabbing );
        }

        if (dam >= 5 && has_artifact_with(AEP_SAP_LIFE)) {
            healall( rng(dam / 10, dam / 5) );
        }

        if( g->u.sees( t ) ) {
            message = melee_message( technique, *this, dealt_dam );
            player_hit_message( this, message, t, dam, critical_hit );
        } else {
            add_msg_player_or_npc( m_good, _("You hit something."), _("<npcname> hits something.") );
        }

        if (!specialmsg.empty()) {
            add_msg_if_player(specialmsg.c_str());
        }

        t.check_dead_state();
    }

    int melee = get_skill_level("melee");
    int mod_sta = ( (weapon.weight() / (12 * str_cur) ) - melee + 20) * -1;
    mod_stat("stamina", mod_sta);
    int sta_percent = (100 * stamina) / get_stamina_max();
    int mod_mc = ( (sta_percent < 25) ? ((25 - sta_percent) * 2) : 0 );
    move_cost += mod_mc;

    mod_moves(-move_cost);

    ma_onattack_effects(); // trigger martial arts on-attack effects
    // some things (shattering weapons) can harm the attacking creature.
    check_dead_state();
    return;
}

void player::reach_attack( const tripoint &p )
{
    matec_id force_technique = tec_none;
    if( weapon.has_flag( "WHIP" ) && ( skillLevel( "melee" ) > 5) && one_in( 3 ) ) {
        force_technique = matec_id( "WHIP_DISARM" );
    }

    Creature *critter = g->critter_at( p );
    // Original target size, used when there are monsters in front of our target
    int target_size = critter != nullptr ? critter->get_size() : 2;

    int move_cost = attack_speed( *this );
    int skill = std::min( 10, (int)get_skill_level("stabbing") );
    int t = 0;
    std::vector<tripoint> path = line_to( pos3(), p, t, 0 );
    path.pop_back(); // Last point is our critter
    for( const tripoint &p : path ) {
        // Possibly hit some unintended target instead
        Creature *inter = g->critter_at( p );
        if( inter != nullptr && 
              !x_in_y( ( target_size * target_size + 1 ) * skill, 
                       ( inter->get_size() * inter->get_size() + 1 ) * 10 ) ) {
            // Even if we miss here, low roll means weapon is pushed away or something like that
            critter = inter;
            break;
        } else if( g->m.move_cost( p ) == 0 &&
                   // Fences etc. Spears can stab through those
                     !( weapon.has_flag( "SPEAR" ) && 
                        g->m.has_flag( "THIN_OBSTACLE", p ) && 
                        x_in_y( skill, 10 ) ) ) {
            g->m.bash( p, str_cur + weapon.type->melee_dam );
            handle_melee_wear();
            mod_moves( -move_cost );
            return;
        }
    }

    if( critter == nullptr ) {
        add_msg_if_player( _("You swing at the air.") );
        if( has_miss_recovery_tec() ) {
            move_cost /= 3; // "Probing" is faster than a regular miss
        }

        mod_moves( -move_cost );
        return;
    }

    melee_attack( *critter, !force_technique.str().empty(), force_technique );
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

 int unarmed_skill = get_skill_level("unarmed");
 int bashing_skill = get_skill_level("bashing");
 int cutting_skill = get_skill_level("cutting");
 int stabbing_skill = get_skill_level("stabbing");
 int melee_skill = get_skill_level("melee");

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
 if (rng(0, 99) < chance)
   num_crits++;

// Dexterity to-hit roll
// ... except sometimes we don't use dexterity!
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
 if (rng(0, 99) < chance)
  num_crits++;

 if (num_crits == 3)
  return true;
 else if (num_crits == 2)
  return (hit_roll() >= target_dodge * 1.5 && !one_in(4));

 return false;
}

int player::get_dodge_base() const {
    // Creature::get_dodge_base includes stat calculations already
    return Creature::get_dodge_base() + get_skill_level("dodge");
}

//Returns 1/2*DEX + dodge skill level + static bonuses from mutations
//Return numbers range from around 4 (starting player, no boosts) to 29 (20 DEX, 10 dodge, +9 mutations)
int player::get_dodge() const
{
    //If we're asleep or busy we can't dodge
    if (in_sleep_state()) {
        return 0;
    }
    if (activity.type != ACT_NULL) {return 0;}

    int ret = Creature::get_dodge();
    // Chop in half if we are unable to move
    if (has_effect("beartrap") || has_effect("lightsnare") || has_effect("heavysnare")) {
        ret /= 2;
    }
    return ret;
}

int player::dodge_roll()
{
    if ( (shoe_type_count("roller_blades") == 2 && one_in((get_dex() + get_skill_level("dodge")) / 3 )) ||
          (shoe_type_count("roller_blades") == 1 && one_in((get_dex() + get_skill_level("dodge")) / 8 ))) {
        if (!has_effect("downed")) {
            // Skaters have a 67% chance to avoid knockdown, and get up a turn quicker.
            if (has_trait("PROF_SKATER")) {
                if (one_in(3)) {
                    if (!has_effect("downed")) {
                        add_msg_if_player(m_bad, _("You overbalance and stumble!"));
                        add_effect("downed", 2);
                    }
                }
                else {
                    add_msg_if_player(m_good, _("You nearly fall, but recover thanks to your skating experience."));
                }
            } else {
                if (!has_effect("downed")) {
                    add_msg_if_player(_("Fighting on wheels is hard!"));
                    add_effect("downed", 3);
                }
            }
        }
    }
    //Fighting on a pair of quad skates isn't so hard, but fighting while wearing a single skate is.
    if (shoe_type_count("rollerskates") == 1 && one_in((get_dex() + get_skill_level("dodge")) / 8 )) {
        if (has_trait("PROF_SKATER")) {
            if (one_in(3)) {
                if (!has_effect("downed")) {
                    add_msg_if_player(m_bad, _("You overbalance and stumble!"));
                    add_effect("downed", 2);
                }
            }
            else {
                add_msg_if_player(m_good, _("You nearly fall, but recover thanks to your skating experience."));
            }
        } else {
            if (!has_effect("downed")) {
                add_msg_if_player(_("Fighting on wheels is hard!"));
                add_effect("downed", 3);
            }
        }
    }
    if (has_effect("bouldering")) {
        if(one_in(get_dex())) {
            add_msg_if_player(m_bad, _("You slip as the ground shifts beneath your feet!"));
            add_effect("downed", 3);
            return 0;
        }
    }
    int dodge_stat = get_dodge();

    if (dodges_left <= 0) { // We already dodged this turn
        if (rng(0, get_skill_level("dodge") + dex_cur + 15) <= get_skill_level("dodge") + dex_cur) {
            dodge_stat = rng(dodge_stat/2, dodge_stat); //Penalize multiple dodges per turn
        } else {
            dodge_stat = 0;
        }
    }

    return dice(dodge_stat, 10); //Matches NPC and monster dodge_roll functions
}

int player::base_damage(bool real_life, int stat)
{
    int dam = 0;
    if( real_life ) {
        if (stat == -999) {
            stat = get_str();
        }

        dam += rng(0, stat / 2);
    } else {
        if (stat == -999) {
            stat = str_max;
        }

        dam += stat / 2;
    }

    // Bonus for strong characters
    if( stat > 10 ) {
        dam += int( (stat - 9) / 2 );
    }
    // Big bonus for super-human characters
    if (stat > 20) {
        dam += int( (stat - 20) * 1.5 );
    }

    return dam;
}

void player::roll_bash_damage( bool crit, damage_instance &di )
{
    int bash_dam = 0;
    int stat = get_str();

    int bashing_skill = get_skill_level("bashing");
    int unarmed_skill = get_skill_level("unarmed");

    if (has_active_bionic("bio_cqb")) {
        bashing_skill = 5;
        unarmed_skill = 5;
    }

    stat += mabuff_bash_bonus();

    int skill = bashing_skill; // Which skill determines damage?
    if( unarmed_attack() ) {
        skill = unarmed_skill;
    }

    bash_dam = base_damage( true, stat );

    // Drunken Master damage bonuses
    if (has_trait("DRUNKEN") && has_effect("drunk")) {
        // Remember, a single drink gives 600 levels of "drunk"
        int mindrunk, maxdrunk;
        int drunk_dur = get_effect_dur("drunk");
        if (unarmed_attack()) {
            mindrunk = drunk_dur / 600;
            maxdrunk = drunk_dur / 250;
        } else {
            mindrunk = drunk_dur / 900;
            maxdrunk = drunk_dur / 400;
        }
        bash_dam += rng(mindrunk, maxdrunk);
    }

    int weap_dam = int(stat / 2) + weapon.damage_bash();
    int bash_cap = 5 + stat + skill;
    float bash_mul = 1.0f;

    if (unarmed_attack()) {
        weap_dam = rng( 0, int(stat / 2) + unarmed_skill + weapon.damage_bash() );
    } else {
        // 80%, 88%, 96%, 104%, 112%, 116%, 120%, 124%, 128%, 132%
        if( bashing_skill < 5 ) {
            bash_mul = 0.8 + 0.08 * bashing_skill;
        } else {
            bash_mul = 0.96 + 0.04 * bashing_skill;
        }
    }

    if( crit ) {
        bash_cap *= 2;
    }

    if( weap_dam > bash_cap ) { // Cap for weak characters
        weap_dam = (bash_cap + weap_dam) / 2;
    }

    int bash_min = weap_dam / 4;

    weap_dam = rng(bash_min, weap_dam);

    if( weap_dam < skill + int(stat / 2) ) {
        weap_dam = rng(weap_dam, skill + int(stat / 2));
    }

    bash_dam += weap_dam;
    bash_mul *= mabuff_bash_mult();

    float armor_mult = 1.0f;
    // Finally, extra crit effects
    if( crit ) {
        bash_dam += int(stat / 2);
        bash_dam += skill;
        bash_mul *= 1.5f;
        // 50% arpen
        armor_mult = 0.5f;
    }
    
    di.add_damage( DT_BASH, bash_dam, 0, armor_mult, bash_mul );
}

void player::roll_cut_damage( bool crit, damage_instance &di )
{
    if( weapon.has_flag("SPEAR") || weapon.has_flag("STAB") ) {
        return;  // Stabs, doesn't cut!
    }

    float cut_dam = mabuff_cut_bonus() + weapon.damage_cut();
    float cut_mul = 1.0f;

    int cutting_skill = get_skill_level("cutting");
    int unarmed_skill = get_skill_level("unarmed");

    if (has_active_bionic("bio_cqb")) {
        cutting_skill = 5;
    }

    if( unarmed_attack() ) {
        // TODO: 1-handed weapons that aren't unarmed attacks
        const bool left_empty = !wearing_something_on(bp_hand_l);
        const bool right_empty = !wearing_something_on(bp_hand_r) && !weapon.has_flag("UNARMED_WEAPON");
        if( left_empty || right_empty ) {
            float per_hand = 0.0f;
            if (has_trait("CLAWS") || (has_active_mutation("CLAWS_RETRACT")) ) {
                per_hand += 3;
            }
            if (has_bionic("bio_razors")) {
                per_hand += 2;
            }
            if (has_trait("TALONS")) {
                per_hand += 3 + (unarmed_skill > 8 ? 4 : unarmed_skill / 2);
            }
            // Stainless Steel Claws do stabbing damage, too.
            if (has_trait("CLAWS_RAT") || has_trait("CLAWS_ST")) {
                per_hand += 1 + (unarmed_skill > 8 ? 4 : unarmed_skill / 2);
            }
            //TODO: add acidproof check back to slime hands (probably move it elsewhere)
            if (has_trait("SLIME_HANDS")) {
                per_hand += rng(2, 3);
            }

            cut_dam += per_hand; // First hand
            if( left_empty && right_empty ) {
                // Second hand
                cut_dam += per_hand;
            }
        }
    }

    if( cut_dam <= 0 ) {
        return; // No negative damage!
    }

    int arpen = 0;

    // 80%, 88%, 96%, 104%, 112%, 116%, 120%, 124%, 128%, 132%
    if( cutting_skill < 5 ) {
        cut_mul *= 0.8 + 0.08 * cutting_skill;
    } else {
        cut_mul *= 0.96 + 0.04 * cutting_skill;
    }

    cut_mul *= mabuff_cut_mult();
    if( crit ) {
        cut_mul *= 1.0 + (cutting_skill / 12.0);
        arpen += 5;
    }

    di.add_damage( DT_CUT, cut_dam, arpen, 0.0f, cut_mul );
}

void player::roll_stab_damage( bool crit, damage_instance &di )
{
    double cut_dam = 0;

    int unarmed_skill = get_skill_level("unarmed");
    int stabbing_skill = get_skill_level("stabbing");
    float stab_mul = 0.0f;

    if (has_active_bionic("bio_cqb")) {
        stabbing_skill = 5;
    }
    if (weapon.has_flag("SPEAR") || weapon.has_flag("STAB")) {
        cut_dam = weapon.damage_cut();
    }
    if( unarmed_attack() ) {
        const bool left_empty = !wearing_something_on(bp_hand_l);
        const bool right_empty = !wearing_something_on(bp_hand_r) && !weapon.has_flag("UNARMED_WEAPON");
        if( left_empty || right_empty ) {
            float per_hand = 0.0f;
            if (has_trait("CLAWS") || has_active_mutation("CLAWS_RETRACT")) {
                per_hand += 3;
            }

            if (has_trait("NAILS")) {
                per_hand += .5;
            }

            if (has_bionic("bio_razors")) {
                per_hand += 2;
            }

            if (has_trait("THORNS")) {
                per_hand += 2;
            }

            if (has_trait("CLAWS_ST")) {
                per_hand += 3 + (unarmed_skill / 2);
            }

            cut_dam += per_hand; // First hand
            if( left_empty && right_empty ) {
                // Second hand
                cut_dam += per_hand;
            }
        }
    }

    if( cut_dam <= 0 ) {
        return; // No negative stabbing!
    }

    // 66%, 76%, 86%, 96%, 106%, 116%, 122%, 128%, 134%, 140%
    if( stabbing_skill <= 5 ) {
        stab_mul *= 0.66 + 0.1 * stabbing_skill;
    } else {
        stab_mul *= 0.86 + 0.06 * stabbing_skill;
    }

    stab_mul *= mabuff_cut_mult();
    float armor_mult = 1.0f;

    if( crit ) {
        stab_mul *= 1.0 + (stabbing_skill / 10.0);
        // Stab criticals have extra extra %arpen
        armor_mult = 0.66f;
    }

    di.add_damage( DT_STAB, cut_dam, 0, armor_mult, stab_mul );
}

// Chance of a weapon sticking is based on weapon attack type.
// Only an issue for cutting and piercing weapons.
// Attack modes are "CHOP", "STAB", and "SLICE".
// "SPEAR" is synonymous with "STAB".
// Weapons can have a "low_stick" flag indicating they
// Have a feature to prevent sticking, such as a spear with a crossbar,
// Or a stabbing blade designed to resist sticking.
int player::roll_stuck_penalty(bool stabbing, const ma_technique &tec)
{
    (void)tec;
    // The cost of the weapon getting stuck, in units of move points.
    const int weapon_speed = attack_speed(*this);
    int stuck_cost = weapon_speed;
    int attack_skill = stabbing ? get_skill_level("stabbing") : get_skill_level("cutting");

    if (has_active_bionic("bio_cqb")) {
        attack_skill = 5;
    }

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
    else if( weapon.has_flag("STAB"))
    {
        // Stabbing has a moderate change of sticking.
        stuck_cost *= 0.50;
    }
    else if( weapon.has_flag("SPEAR"))
    {
        // Spears should be a bit easier to manage
        stuck_cost *= 0.25;
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
    // And also strength. This time totally reliable 5 moves per point
    stuck_cost -= 5 * str_cur;
    // Make sure cost doesn't go negative.
    stuck_cost = std::max( stuck_cost, 0 );

    // Cap stuck penalty at 2x weapon speed
    stuck_cost = std::min( stuck_cost, 2 * weapon_speed );

    return stuck_cost;
}

matec_id player::pick_technique(Creature &t,
                                bool crit, bool dodge_counter, bool block_counter)
{

    std::vector<matec_id> all = get_all_techniques();

    std::vector<matec_id> possible;

    bool downed = t.has_effect("downed");

    // first add non-aoe tecs
    for( auto & tec_id : all ) {
        const ma_technique &tec = tec_id.obj();

        // ignore "dummy" techniques like WBLOCK_1
        if (tec.dummy) {
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
        // dice(   dex_cur +    get_skill_level("unarmed"),  8) >
        // dice(p->dex_cur + p->get_skill_level("melee"),   10))
        if (tec.disarms && !t.has_weapon()) {
            continue;
        }

        // if aoe, check if there are valid targets
        if (tec.aoe.length() > 0 && !valid_aoe_technique(t, tec)) {
            continue;
        }

        // If we have negative weighting then roll to see if it's valid this time
        if (tec.weighting < 0 && !one_in(abs(tec.weighting))) {
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
        return tec_none;
    }

    return possible[ rng(0, possible.size() - 1) ];
}

bool player::valid_aoe_technique( Creature &t, const ma_technique &technique )
{
    std::vector<int> dummy_mon_targets;
    std::vector<int> dummy_npc_targets;
    return valid_aoe_technique( t, technique, dummy_mon_targets, dummy_npc_targets );
}

bool player::valid_aoe_technique( Creature &t, const ma_technique &technique,
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

        int lookup = t.posy() - posy() + 1 + (3 * (t.posx() - posx() + 1));

        tripoint left = pos3() + tripoint( offset_a[lookup], offset_b[lookup], 0 );
        tripoint right = pos3() + tripoint( offset_b[lookup], -offset_a[lookup], 0 );

        int mondex_l = g->mon_at( left );
        int mondex_r = g->mon_at( right );
        if (mondex_l != -1 && g->zombie(mondex_l).friendly == 0) {
            mon_targets.push_back(mondex_l);
        }
        if (mondex_r != -1 && g->zombie(mondex_r).friendly == 0) {
            mon_targets.push_back(mondex_r);
        }

        int npcdex_l = g->npc_at( left );
        int npcdex_r = g->npc_at( right );
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

        int lookup = t.posy() - posy() + 1 + (3 * (t.posx() - posx() + 1));

        tripoint left = t.pos3() + tripoint( offset_a[lookup], offset_b[lookup], 0 );
        tripoint target_pos = t.pos3() + (t.pos3() - pos3());
        tripoint right = t.pos3() + tripoint( offset_b[lookup], -offset_b[lookup], 0 );

        int mondex_l = g->mon_at( left );
        int mondex_t = g->mon_at( target_pos );
        int mondex_r = g->mon_at( right );
        if (mondex_l != -1 && g->zombie(mondex_l).friendly == 0) {
            mon_targets.push_back(mondex_l);
        }
        if (mondex_t != -1 && g->zombie(mondex_t).friendly == 0) {
            mon_targets.push_back(mondex_t);
        }
        if (mondex_r != -1 && g->zombie(mondex_r).friendly == 0) {
            mon_targets.push_back(mondex_r);
        }

        int npcdex_l = g->npc_at( left );
        int npcdex_t = g->npc_at( target_pos );
        int npcdex_r = g->npc_at( right );
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
        tripoint tmp;
        tmp.z = posz();
        for ( tmp.x = posx() - 1; tmp.x <= posx() + 1; tmp.x++) {
            for ( tmp.y = posy() - 1; tmp.y <= posy() + 1; tmp.y++) {
                if( tmp == t.pos3() ) {
                    continue;
                }
                int mondex = g->mon_at( tmp );
                if (mondex != -1 && g->zombie(mondex).friendly == 0) {
                    mon_targets.push_back(mondex);
                }
                int npcdex = g->npc_at( tmp );
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

bool player::has_technique( const matec_id &id ) const
{
    return weapon.has_technique( id ) ||
           style_selected.obj().has_technique( *this, id );
}

damage_unit &get_damage_unit( std::vector<damage_unit> &di, const damage_type dt )
{
    static damage_unit nullunit( DT_NULL, 0, 0, 0, 0 );
    for( auto &du : di ) {
        if( du.type == dt && du.amount > 0 ) {
            return du;
        }
    }

    return nullunit;
}

void player::perform_technique(const ma_technique &technique, Creature &t, damage_instance &di, int &move_cost)
{
    auto bash = get_damage_unit( di.damage_units, DT_BASH );
    auto cut  = get_damage_unit( di.damage_units, DT_CUT );
    auto stab = get_damage_unit( di.damage_units, DT_STAB );

    if( bash.amount > 0 ) {
        bash.amount += technique.bash;
        bash.damage_multiplier *= technique.bash_mult;
    }

    // Cut affects stab damage too since only one of cut/stab is used
    if( cut.amount > 0 && cut.amount > stab.amount ) {
        cut.amount += technique.cut;
        bash.damage_multiplier *= technique.cut_mult;
    } else if( stab.amount > 0 ) {
        stab.amount += technique.cut;
        bash.damage_multiplier *= technique.cut_mult;
    }

    move_cost *= technique.speed_mult;

    if( technique.down_dur > 0 ) {
        if( t.get_throw_resist() == 0 ) {
            t.add_effect("downed", rng(1, technique.down_dur));
            if( bash.amount > 0 ) {
                bash.amount += 3;
            }
        }
    }

    if (technique.stun_dur > 0) {
        t.add_effect("stunned", rng(1, technique.stun_dur));
    }

    if (technique.knockback_dist > 0) {
        const int kb_offset_x = rng( -technique.knockback_spread,
                                     technique.knockback_spread );
        const int kb_offset_y = rng( -technique.knockback_spread,
                                     technique.knockback_spread );
        tripoint kb_point( posx() + kb_offset_x, posy() + kb_offset_y, posz() );
        t.knock_back_from( kb_point );
    }

    if (technique.pain > 0) {
        t.pain += rng(technique.pain / 2, technique.pain);
    }

    player *p = dynamic_cast<player*>( &t );
    if( technique.disarms && p != nullptr && p->is_armed() ) {
        g->m.add_item_or_charges( p->pos3(), p->remove_weapon() );
        if( p->is_player() ) {
            add_msg_if_npc( _("<npcname> disarms you!") );
        } else {
            add_msg_player_or_npc( _("You disarm %s!"),
                                   _("<npcname> disarms %s!"),
                                   p->name.c_str() );
        }
    }

    //AOE attacks, feel free to skip over this lump
    if (technique.aoe.length() > 0) {
        int temp_moves = moves;

        std::vector<int> mon_targets = std::vector<int>();
        std::vector<int> npc_targets = std::vector<int>();

        valid_aoe_technique( t, technique, mon_targets, npc_targets );

        //hit only one valid target (pierce through doesn't spread out)
        if (technique.aoe == "impale") {
            size_t victim = rng(0, mon_targets.size() + npc_targets.size() - 1);
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
        for (auto &i : mon_targets) {
            if (hit_roll() >= rng(0, 5) + g->zombie(i).dodge_roll()) {
                count_hit++;
                melee_attack(g->zombie(i), false);

                std::string temp_target = string_format(_("the %s"), g->zombie(i).name().c_str());
                add_msg_player_or_npc( m_good, _("You hit %s!"), _("<npcname> hits %s!"), temp_target.c_str() );
            }
        }
        for (auto &i : npc_targets) {
            if (hit_roll() >= rng(0, 5) + g->active_npc[i]->dodge_roll()) {
                count_hit++;
                melee_attack(*g->active_npc[i], false);

                add_msg_player_or_npc( m_good, _("You hit %s!"), _("<npcname> hits %s!"),
                                          g->active_npc[i]->name.c_str() );
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
                       style_selected.obj().name.c_str());
        }
    }
}

// this would be i2amroy's fix, but it's kinda handy
bool player::can_weapon_block()
{
    return (weapon.has_technique( WBLOCK_1 ) ||
            weapon.has_technique( WBLOCK_2 ) ||
            weapon.has_technique( WBLOCK_3 ));
}

void player::dodge_hit(Creature *source, int) {
    if( dodges_left < 1 ) {
        return;
    }

    ma_ondodge_effects(); // fire martial arts block-triggered effects

    dodges_left--;

    practice( "dodge", source->get_melee() * 2, source->get_melee() );

    // check if we have any dodge counters
    matec_id tec = pick_technique(*source, false, true, false);

    if( tec != tec_none ) {
        melee_attack(*source, true, tec);
    }
}

bool player::block_hit(Creature *source, body_part &bp_hit, damage_instance &dam) {

    //Shouldn't block if player is asleep; this only seems to be used by player.

    if (blocks_left < 1 || in_sleep_state()) {
        return false;
    }
    blocks_left--;

    ma_ongethit_effects(); // fire martial arts on-getting-hit-triggered effects
    // these fire even if the attack is blocked (you still got hit)

    // This bonus absorbs damage from incoming attacks before they land,
    // but it still counts as a block even if it absorbs all the damage.
    float total_phys_block = mabuff_block_bonus();
    bool conductive_weapon = weapon.conductive();

    // This gets us a number between:
    // str ~0 + skill 0 = 0
    // str ~20 + skill 10 + 10(unarmed skill or weapon bonus) = 40
    int block_score = 1;
    if (can_weapon_block()) {
        int block_bonus = 2;
        if (weapon.has_technique( WBLOCK_3 )) {
            block_bonus = 10;
        } else if (weapon.has_technique( WBLOCK_2 )) {
            block_bonus = 6;
        } else if (weapon.has_technique( WBLOCK_1 )) {
            block_bonus = 4;
        }
        block_score = str_cur + block_bonus + (int)get_skill_level("melee");
    } else if (can_limb_block()) {
        block_score = str_cur + (int)get_skill_level("melee") + (int)get_skill_level("unarmed");
    }

    // Map block_score to the logistic curve for a number between 1 and 0.
    // Basic beginner character (str 8, skill 0, basic weapon)
    // Will have a score around 10 and block about %15 of incoming damage.
    // More proficient melee character (str 10, skill 4, wbock_2 weapon)
    // will have a score of 20 and block about 45% of damage.
    // A highly expert character (str 14, skill 8 wblock_2)
    // will have a score in the high 20s and will block about 80% of damage.
    // As the block score approaches 40, damage making it through will dwindle
    // to nothing, at which point we're relying on attackers hitting enough to drain blocks.
    const float physical_block_multiplier = player::logistic_range( 0, 40, block_score );

    float total_damage = 0.0;
    float damage_blocked = 0.0;
    for( auto &elem : dam.damage_units ) {
        total_damage += elem.amount;
        // block physical damage "normally"
        if( elem.type == DT_BASH || elem.type == DT_CUT || elem.type == DT_STAB ) {
            // use up our flat block bonus first
            float block_amount = std::min( total_phys_block, elem.amount );
            total_phys_block -= block_amount;
            elem.amount -= block_amount;
            damage_blocked += block_amount;
            if( elem.amount <= std::numeric_limits<float>::epsilon() ) {
                continue;
            }

            float previous_amount = elem.amount;
            elem.amount *= physical_block_multiplier;
            damage_blocked += previous_amount - elem.amount;
        // non-electrical "elemental" damage types do their full damage if unarmed,
        // but severely mitigated damage if not
        } else if( elem.type == DT_HEAT || elem.type == DT_ACID || elem.type == DT_COLD ) {
            //TODO: should damage weapons if blocked
            if (!unarmed_attack() && can_weapon_block()) {
                float previous_amount = elem.amount;
                elem.amount /= 5;
                damage_blocked += previous_amount - elem.amount;
            }
        // electrical damage deals full damage if unarmed OR wielding a
        // conductive weapon
        } else if( elem.type == DT_ELECTRIC ) {
            if (!unarmed_attack() && can_weapon_block() && !conductive_weapon) {
                float previous_amount = elem.amount;
                elem.amount /= 5;
                damage_blocked += previous_amount - elem.amount;
            }
        }
    }

    ma_onblock_effects(); // fire martial arts block-triggered effects

    // weapon blocks are preferred to arm blocks
    std::string thing_blocked_with;
    if (player::is_armed() && !weapon.has_flag("UNARMED_WEAPON")) {
        thing_blocked_with = weapon.tname();
        handle_melee_wear();
    } else {
        //Choose which body part to block with, assume left side first
        if (can_leg_block() && can_arm_block()) {
            bp_hit = one_in(2) ? bp_leg_l : bp_arm_l;
        } else if (can_leg_block()) {
            bp_hit = bp_leg_l;
        } else {
            bp_hit = bp_arm_l;
        }

        // Check if we should actually use the right side to block
        if (bp_hit == bp_leg_l) {
            if (hp_cur[hp_leg_r] > hp_cur[hp_leg_l]) {
                bp_hit = bp_leg_r;
            }
        } else {
            if (hp_cur[hp_arm_r] > hp_cur[hp_arm_l]) {
                bp_hit = bp_arm_r;
            }
        }

        thing_blocked_with = body_part_name(bp_hit);
    }

    std::string damage_blocked_description;
    // good/bad/ugly add_msg color code?
    // none, hardly any, a little, some, most, all
    float blocked_ratio = (total_damage - damage_blocked) / total_damage;
    if( blocked_ratio < std::numeric_limits<float>::epsilon() ) {
        //~ Adjective in "You block <adjective> of the damage with your <weapon>.
        damage_blocked_description = _("all");
    } else if( blocked_ratio < 0.2 ) {
        //~ Adjective in "You block <adjective> of the damage with your <weapon>.
        damage_blocked_description = _("nearly all");
    } else if( blocked_ratio < 0.4 ) {
        //~ Adjective in "You block <adjective> of the damage with your <weapon>.
        damage_blocked_description = _("most");
    } else if( blocked_ratio < 0.6 ) {
        //~ Adjective in "You block <adjective> of the damage with your <weapon>.
        damage_blocked_description = _("a lot");
    } else if( blocked_ratio < 0.8 ) {
        //~ Adjective in "You block <adjective> of the damage with your <weapon>.
        damage_blocked_description = _("some");
    } else if( blocked_ratio > std::numeric_limits<float>::epsilon() ){
        //~ Adjective in "You block <adjective> of the damage with your <weapon>.
        damage_blocked_description = _("a little");
    } else {
        //~ Adjective in "You block <adjective> of the damage with your <weapon>.
        damage_blocked_description = _("none");
    }
    add_msg_player_or_npc( _("You block %s of the damage with your %s!"),
                           _("<npcname> blocks %s of the damage with their %s!"),
                           damage_blocked_description.c_str(), thing_blocked_with.c_str() );

    // check if we have any dodge counters
    matec_id tec = pick_technique(*source, false, false, true);

    if( tec != tec_none ) {
        melee_attack(*source, true, tec);
    }

    return true;
}

void player::perform_special_attacks(Creature &t)
{
 bool can_poison = false;

 std::vector<special_attack> special_attacks = mutation_attacks(t);

 std::string target = t.disp_name();

 for( auto &special_attack : special_attacks ) {
  dealt_damage_instance dealt_dam;

  int hit_spread = t.deal_melee_attack(this, hit_roll() * 0.8);
  if (hit_spread >= 0)
      t.deal_melee_hit(
          this, hit_spread, false,
          damage_instance::physical( special_attack.bash, special_attack.cut, special_attack.stab ),
          dealt_dam );
  if (dealt_dam.total_damage() > 0)
      add_msg_if_player( m_good, special_attack.text.c_str() );

  if (!can_poison && (dealt_dam.type_damage(DT_CUT) > 0 ||
        dealt_dam.type_damage(DT_STAB) > 0 ))
   can_poison = true;
 }

 if (can_poison && ((has_trait("POISONOUS")) || (has_trait("POISONOUS2")))) {
    if (has_trait("POISONOUS")) {
        t.add_msg_if_player(m_good, _("You poison %s!"), target.c_str());
        t.add_effect("poison", 6);
    }
    else if (has_trait("POISONOUS2")) {
        t.add_msg_if_player(m_good, _("You inject your venom into %s!"), target.c_str());
        t.add_effect("nasty_poison", 6);
    }
 }
}

std::string player::melee_special_effects(Creature &t, damage_instance &d, const ma_technique &tec)
{
    std::stringstream dump;

    std::string target;

    target = t.disp_name();

    tripoint tarpos = t.pos3();

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
        charge_power(-2);
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
        deal_damage(NULL, bp_hand_r, damage_instance::physical(0, weapon.damage_cut(), 0));
        if (weapon.is_two_handed(this)) { // Hurt left hand too, if it was big
            deal_damage(NULL, bp_hand_l, damage_instance::physical(0, weapon.damage_cut(), 0));
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

        sounds::sound( pos3(), 16, "" );
        // Dump its contents on the ground
        for( auto &elem : weapon.contents ) {
            g->m.add_item_or_charges( pos3(), elem );
        }
        // Take damage
        deal_damage(this, bp_arm_r, damage_instance::physical(0, rng(0, weapon.volume() * 2), 0));
        if (weapon.is_two_handed(this)) {// Hurt left arm too, if it was big
            //redeclare shatter_dam because deal_damage mutates it
            deal_damage(this, bp_arm_l, damage_instance::physical(0, rng(0, weapon.volume() * 2), 0));
        }
        d.add_damage(DT_CUT, rng(0, 5 + int(weapon.volume() * 1.5)));// Hurt the monster extra
        remove_weapon();
    }

    handle_melee_wear();

    bool is_hallucination = false; //Check if the target is an hallucination.
    if(monster *m = dynamic_cast<monster *>(&t)) { //Cast fails if the t is an NPC or the player.
        is_hallucination = m->is_hallucination();
    }

    // The skill used to counter stuck penalty
    const bool stab = d.type_damage(DT_STAB) > d.type_damage(DT_CUT);
    int used_skill = stab ? get_skill_level("stabbing") : get_skill_level("cutting");
    if( has_active_bionic("bio_cqb") ) {
        used_skill = 5;
    }

    int cutting_penalty = roll_stuck_penalty( stab, tec );
    // Some weapons splatter a lot of blood around.
    // TODO: this function shows total damage done by this attack, not final damage inflicted.
    if (d.total_damage() > 10 && weapon.has_flag("MESSY") ) { //Check if you do non minor damage
        cutting_penalty /= 6; // Harder to get stuck
        //Get blood type.
        field_id type_blood = fd_blood;
        if(!is_hallucination || t.has_flag(MF_VERMIN)) {
            type_blood = t.bloodType();
        } else {
            type_blood = fd_null;
        }
        if(type_blood != fd_null) {
            tripoint tmp;
            tmp.z = tarpos.z;
            for( tmp.x = tarpos.x - 1; tmp.x <= tarpos.x + 1; tmp.x++ ) {
                for( tmp.y = tarpos.y - 1; tmp.y <= tarpos.y + 1; tmp.y++ ) {
                    if( !one_in(3) ) {
                        g->m.add_field( tmp, type_blood, 1, 0 );
                    }
                }
            }
        }
    }
    // Getting your weapon stuck
    if (!unarmed_attack() && cutting_penalty > dice(str_cur * 2, 20) && !is_hallucination) {
        dump << string_format(_("Your %s gets stuck in %s, pulling it out of your hands!"),
                              weapon.tname().c_str(), target.c_str());
        // TODO: better speed debuffs for target, possibly through effects
        if (monster *m = dynamic_cast<monster *>(&t)) {
            m->add_item(remove_weapon());
        } else {
            // Happens if 't' is not of 'monster' origin (attacking an NPC)
            g->m.add_item_or_charges( tarpos, remove_weapon(), 1 );
        }

        t.mod_moves(-30);
        if (weapon.has_flag("HURT_WHEN_PULLED") && one_in(3)) {
            //Sharp objects that injure wielder when pulled from hands (so cutting damage only)
            dump << std::endl << string_format(_("You are hurt by the %s being pulled from your hands!"),
                                               weapon.tname().c_str());
            auto hand_hurt = one_in(2) ? bp_hand_l : bp_hand_r;
            deal_damage( this, hand_hurt, damage_instance::physical( 0, weapon.damage_cut() / 2, 0) );
        }
    } else {
        // Approximately "this would kill the target". Unless the target has limb-based hp.
        if( d.total_damage() > t.get_hp() ) {
            cutting_penalty /= 2;
            cutting_penalty -= rng( used_skill, used_skill * 2 + 2);
        }
        if( cutting_penalty >= 50 && !is_hallucination ) {
            dump << string_format(_("Your %s gets stuck in %s but you yank it free!"), weapon.tname().c_str(),
                                  target.c_str());
        }

        if( weapon.has_flag("SPEAR") ) {
            const int stabbing_skill = get_skill_level("stabbing");
            d.add_damage( DT_CUT, rng( 1, stabbing_skill ) ); //add some extra damage for pulling out a spear
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
         one_in(20 - dex_cur - get_skill_level("unarmed")) ) {
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

    // Having lupine or croc jaws makes it much easier to sink your fangs into people;
    // Ursine/Feline, not so much.  Rat is marginally better.
    if (has_trait("FANGS") && (!wearing_something_on(bp_mouth)) &&
        ((!has_trait("MUZZLE") && !has_trait("MUZZLE_LONG") && !has_trait("MUZZLE_RAT") &&
          one_in(20 - dex_cur - get_skill_level("unarmed"))) ||
         (has_trait("MUZZLE_RAT") && one_in(19 - dex_cur - get_skill_level("unarmed"))) ||
         (has_trait("MUZZLE") && one_in(18 - dex_cur - get_skill_level("unarmed"))) ||
         (has_trait("MUZZLE_LONG") && one_in(15 - dex_cur - get_skill_level("unarmed"))))) {
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

    if (has_trait("INCISORS") && one_in(18 - dex_cur - get_skill_level("unarmed")) &&
        (!wearing_something_on(bp_mouth))) {
        special_attack tmp;
        tmp.cut = 3;
        tmp.bash = 3;
        if (is_player()) {
            tmp.text = string_format(_("You bite into %s with your ratlike incisors!"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%s bites %s with his ratlike incisors!"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%s bites %s with her ratlike incisors!"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    if (!has_trait("FANGS") && has_trait("MUZZLE") &&
        one_in(18 - dex_cur - get_skill_level("unarmed")) &&
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
        one_in(20 - dex_cur - get_skill_level("unarmed")) &&
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
        one_in(18 - dex_cur - get_skill_level("unarmed")) &&
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

    if ((has_trait("MANDIBLES") || (has_trait("FANGS_SPIDER") && !has_active_mutation("FANGS_SPIDER"))) &&
        one_in(22 - dex_cur - get_skill_level("unarmed")) && (!wearing_something_on(bp_mouth))) {
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
    if (has_active_mutation("FANGS_SPIDER") && one_in(24 - dex_cur - get_skill_level("unarmed")) &&
        (!wearing_something_on(bp_mouth)) ) {
        special_attack tmp;
        tmp.stab = 15;
        if (is_player()) {
            tmp.text = string_format(_("You bite %s with your fangs!"),
                                     target.c_str());
        } else if (male) {
            tmp.text = string_format(_("%s bites %s with his fangs!"),
                                     name.c_str(), target.c_str());
        } else {
            tmp.text = string_format(_("%s bites %s with her fangs!"),
                                     name.c_str(), target.c_str());
        }
        ret.push_back(tmp);
    }

    if (has_trait("BEAK") && one_in(15 - dex_cur - get_skill_level("unarmed")) &&
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

    if (has_trait("BEAK_PECK") && one_in(15 - dex_cur - get_skill_level("unarmed")) &&
        (!wearing_something_on(bp_mouth))) {
        // method open to improvement, please feel free to suggest
        // a better way to simulate target's anti-peck efforts
        int num_hits = (dex_cur + get_skill_level("unarmed") - rng(4, 10));
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

    if (has_trait("HOOVES") && one_in(25 - dex_cur - 2 * get_skill_level("unarmed"))) {
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

    if (has_trait("RAP_TALONS") && one_in(30 - dex_cur - 2 * get_skill_level("unarmed"))) {
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

    if (has_trait("HORNS") && one_in(20 - dex_cur - get_skill_level("unarmed"))) {
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

    if (has_trait("HORNS_CURLED") && one_in(20 - dex_cur - get_skill_level("unarmed"))) {
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

    if (has_trait("HORNS_POINTED") && one_in(22 - dex_cur - get_skill_level("unarmed"))) {
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

    if (has_trait("ANTLERS") && one_in(20 - dex_cur - get_skill_level("unarmed"))) {
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

    if ( ((has_trait("TAIL_STING") && one_in(3)) || has_active_mutation("TAIL_STING")) &&
      one_in(10 - dex_cur)) {
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

    if ( ((has_trait("TAIL_CLUB") && one_in(3)) || has_active_mutation("TAIL_CLUB")) &&
      one_in(10 - dex_cur)) {
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

    if (((has_trait("TAIL_THICK") && one_in(3)) || has_active_mutation("TAIL_THICK")) &&
      one_in(10 - dex_cur)) {
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

std::string melee_message( const ma_technique &tec, player &p, const dealt_damage_instance &ddi )
{
    const int bash_dam = ddi.type_damage( DT_BASH );
    const int cut_dam  = ddi.type_damage( DT_CUT );
    const int stab_dam = ddi.type_damage( DT_STAB );

    if( tec.id != tec_none ) {
        if (tec.messages.size() < 2) {
            return "The bugs nibble %s";
        } else if (p.is_npc()) {
            return tec.messages[1];
        } else {
            return tec.messages[0];
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
                        Creature &t, int dam, bool crit)
{
    std::string msg;
    game_message_type msgtype;
    msgtype = m_good;
    std::string sSCTmod = "";
    game_message_type gmtSCTcolor = m_good;

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
        sSCTmod = _("Critical!");
        gmtSCTcolor = m_critical;
    } else {
        //~ someone hits something for %d damage
        msg = string_format(_("%s for %d damage."), message.c_str(), dam);
    }

    if (dam > 0 && attacker->is_player()) {
        //player hits monster melee
        SCT.add(t.posx(),
                t.posy(),
                direction_from(0, 0, t.posx() - attacker->posx(), t.posy() - attacker->posy()),
                get_hp_bar(dam, t.get_hp_max(), true).first, m_good,
                sSCTmod, gmtSCTcolor);

        if (t.get_hp() > 0) {
            SCT.add(t.posx(),
                    t.posy(),
                    direction_from(0, 0, t.posx() - attacker->posx(), t.posy() - attacker->posy()),
                    get_hp_bar(t.get_hp(), t.get_hp_max(), true).first, m_good,
                    //~ “hit points”, used in scrolling combat text
                    _("hp"), m_neutral,
                    "hp");
        } else {
            SCT.removeCreatureHP();
        }
    }

    // same message is used for player and npc,
    // just using this for the <npcname> substitution.
    attacker->add_msg_player_or_npc(msgtype, msg.c_str(), msg.c_str(),
                                    t.disp_name().c_str());
}

void melee_practice( player &u, bool hit, bool unarmed,
                     bool bashing, bool cutting, bool stabbing )
{
    int min = 2;
    int max = 2;
    std::string first = "";
    std::string second = "";
    std::string third = "";

    if (hit) {
        min = 5;
        max = 10;
        u.practice( "melee", rng(5, 10) );
    } else {
        u.practice( "melee", rng(2, 5) );
    }

    // type of weapon used determines order of practice
    if (u.weapon.has_flag("SPEAR")) {
        if (stabbing) first  = "stabbing";
        if (bashing)  second = "bashing";
        if (cutting)  third  = "cutting";
    } else if (u.weapon.has_flag("STAB")) {
        // stabbity weapons have a 50-50 chance of raising either stabbing or cutting first
        if (one_in(2)) {
            if (stabbing) first  = "stabbing";
            if (cutting)  second = "cutting";
            if (bashing)  third  = "bashing";
        } else {
            if (cutting)  first  = "cutting";
            if (stabbing) second = "stabbing";
            if (bashing)  third  = "bashing";
        }
    } else if (u.weapon.is_cutting_weapon()) {
        if (cutting)  first  = "cutting";
        if (bashing)  second = "bashing";
        if (stabbing) third  = "stabbing";
    } else {
        if (bashing)  first  = "bashing";
        if (cutting)  second = "cutting";
        if (stabbing) third  = "stabbing";
    }

    if (unarmed) u.practice( "unarmed", rng(min, max) );
    if (!first.empty())  u.practice( first, rng(min, max) );
    if (!second.empty()) u.practice( second, rng(min, max) );
    if (!third.empty())  u.practice( third, rng(min, max) );
}

int attack_speed(player &u)
{
    const int base_move_cost = u.weapon.attack_time() / 2;
    const int melee_skill = u.has_active_bionic("bio_cqb") ? 5 : (int)u.get_skill_level("melee");
    const int skill_cost = (int)( base_move_cost / (std::pow(melee_skill, 3.0f)/400.0 + 1.0));
    const int dexbonus = rng( 0, u.dex_cur );

    int move_cost = base_move_cost;
    move_cost += skill_cost;
    move_cost += 2 * u.encumb(bp_torso);
    move_cost -= dexbonus;

    if( u.has_trait("HOLLOW_BONES") ) {
        move_cost *= .8;
    } else if( u.has_trait("LIGHT_BONES") ) {
        move_cost *= .9;
    }

    if( move_cost < 25 ) {
        return 25;
    }

    return move_cost;
}
