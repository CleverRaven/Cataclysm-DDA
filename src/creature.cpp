#include "item.h"
#include "creature.h"
#include "output.h"
#include "game.h"
#include <algorithm>
#include <numeric>
#include <cmath>

std::map<int, std::map<body_part, double> > Creature::default_hit_weights;

Creature::Creature()
{
    str_max = 0;
    dex_max = 0;
    per_max = 0;
    int_max = 0;
    str_cur = 0;
    dex_cur = 0;
    per_cur = 0;
    int_cur = 0;
    healthy = 0;
    healthy_mod = 0;
    moves = 0;
    pain = 0;
    killer = NULL;
    speed_base = 100;

    reset_bonuses();

    fake = false;
}

Creature::Creature(const Creature &rhs)
{
    str_max = rhs.str_max;
    dex_max = rhs.dex_max;
    per_max = rhs.per_max;
    int_max = rhs.int_max;
    str_cur = rhs.str_cur;
    dex_cur = rhs.dex_cur;
    per_cur = rhs.per_cur;
    int_cur = rhs.int_cur;
    moves = rhs.moves;
    pain = rhs.pain;
    killer = rhs.killer;
    speed_base = rhs.speed_base;

    str_bonus = rhs.str_bonus;
    dex_bonus = rhs.dex_bonus;
    per_bonus = rhs.per_bonus;
    int_bonus = rhs.int_bonus;

    healthy = rhs.healthy;
    healthy_mod = rhs.healthy_mod;

    num_blocks = rhs.num_blocks;
    num_dodges = rhs.num_dodges;
    num_blocks_bonus = rhs.num_blocks_bonus;
    num_dodges_bonus = rhs.num_dodges_bonus;

    armor_bash_bonus = rhs.armor_bash_bonus;
    armor_cut_bonus = rhs.armor_cut_bonus;

    speed_bonus = rhs.speed_bonus;
    dodge_bonus = rhs.dodge_bonus;
    block_bonus = rhs.block_bonus;
    hit_bonus = rhs.hit_bonus;
    bash_bonus = rhs.bash_bonus;
    cut_bonus = rhs.cut_bonus;

    bash_mult = rhs.bash_mult;
    cut_mult = rhs.cut_mult;

    melee_quiet = rhs.melee_quiet;
    grab_resist = rhs.grab_resist;
    throw_resist = rhs.throw_resist;

    effects = rhs.effects;

    fake = rhs.fake;
}

void Creature::normalize()
{
}

void Creature::reset()
{
    reset_bonuses();
    reset_stats();
}
void Creature::reset_bonuses()
{
    // Reset all bonuses to 0 and mults to 1.0
    str_bonus = 0;
    dex_bonus = 0;
    per_bonus = 0;
    int_bonus = 0;

    num_blocks = 1;
    num_dodges = 1;
    num_blocks_bonus = 0;
    num_dodges_bonus = 0;

    armor_bash_bonus = 0;
    armor_cut_bonus = 0;

    speed_bonus = 0;
    dodge_bonus = 0;
    block_bonus = 0;
    hit_bonus = 0;
    bash_bonus = 0;
    cut_bonus = 0;

    bash_mult = 1.0f;
    cut_mult = 1.0f;

    melee_quiet = false;
    grab_resist = 0;
    throw_resist = 0;
}

void Creature::reset_stats()
{
    // Reset our stats to normal levels
    // Any persistent buffs/debuffs will take place in disease.h,
    // player::suffer(), etc.

    // repopulate the stat fields
    process_effects();
    str_cur = str_max + get_str_bonus();
    dex_cur = dex_max + get_dex_bonus();
    per_cur = per_max + get_per_bonus();
    int_cur = int_max + get_int_bonus();

    // Floor for our stats.  No stat changes should occur after this!
    if (dex_cur < 0) {
        dex_cur = 0;
    }
    if (str_cur < 0) {
        str_cur = 0;
    }
    if (per_cur < 0) {
        per_cur = 0;
    }
    if (int_cur < 0) {
        int_cur = 0;
    }

    // add an appropriate number of moves
    moves += get_speed();
}

// MF_DIGS or MF_CAN_DIG and diggable terrain
bool Creature::digging()
{
    return false;
}

/*
 * Damage-related functions
 */


// TODO: this is a shim for the currently existing calls to Creature::hit,
// start phasing them out
int Creature::hit(Creature *source, body_part bphurt, int side,
                  int dam, int cut)
{
    damage_instance d;
    d.add_damage(DT_BASH, dam);
    d.add_damage(DT_CUT, cut);
    dealt_damage_instance dealt_dams = deal_damage(source, bphurt, side, d);

    return dealt_dams.total_damage();
}

int Creature::deal_melee_attack(Creature *source, int hitroll)
{
    int dodgeroll = dodge_roll();
    int hit_spread = hitroll - dodgeroll;
    bool missed = hit_spread <= 0;

    if (missed) {
        dodge_hit(source, hit_spread);
        return hit_spread;
    }

    return hit_spread;
}

void Creature::deal_melee_hit(Creature *source, int hit_spread, bool critical_hit,
                                const damage_instance &dam, dealt_damage_instance &dealt_dam)
{
    damage_instance d = dam; // copy, since we will mutate in block_hit

    body_part bp_hit = select_body_part(source, hit_spread);
    int side = rng(0, 1);
    block_hit(source, bp_hit, side, d);

    // Bashing crit
    if (critical_hit) {
        int turns_stunned = (d.type_damage(DT_BASH) + hit_spread) / 20;
        if (turns_stunned > 6) {
            turns_stunned = 6;
        }
        if (turns_stunned > 0) {
            add_effect("stunned", turns_stunned);
        }
    }

    // Stabbing effects
    int stab_moves = rng(d.type_damage(DT_STAB) / 2, d.type_damage(DT_STAB) * 1.5);
    if (critical_hit) {
        stab_moves *= 1.5;
    }
    if (stab_moves >= 150) {
        if ((is_player()) && ((!(g->u.has_trait("LEG_TENT_BRACE"))) || (g->u.wearing_something_on(bp_feet))) ) {
            // can the player force their self to the ground? probably not.
            g->add_msg_if_npc(source, _("<npcname> forces you to the ground!"));
        } else {
            g->add_msg_player_or_npc(source, _("You force %s to the ground!"),
                                     _("<npcname> forces %s to the ground!"),
                                     disp_name().c_str() );
        }
        if ((!(g->u.has_trait("LEG_TENT_BRACE"))) || (g->u.wearing_something_on(bp_feet)) ) {
            add_effect("downed", 1);
            mod_moves(-stab_moves / 2);
        }
    } else {
        mod_moves(-stab_moves);
    }

    on_gethit(source, bp_hit, d); // trigger on-gethit events
    dealt_dam = deal_damage(source, bp_hit, side, d);
    dealt_dam.bp_hit = bp_hit;
}

int Creature::deal_projectile_attack(Creature *source, double missed_by,
                                     const projectile &proj, dealt_damage_instance &dealt_dam)
{
    bool u_see_this = g->u_see(this);
    body_part bp_hit;
    int side = rng(0, 1);

    // do 10,speed because speed could potentially be > 10000
    if (dodge_roll() >= dice(10, proj.speed)) {
        if (is_player())
            g->add_msg(_("You dodge %s projectile!"),
                       source->disp_name(true).c_str());
        else if (u_see_this)
            g->add_msg(_("%s dodges %s projectile."),
                       disp_name().c_str(), source->disp_name(true).c_str());
        return 0;
    }

    // Bounce applies whether it does damage or not.
    if (proj.proj_effects.count("BOUNCE")) {
        add_effect("bounced", 1);
    }

    double hit_value = missed_by + rng_float(-0.5, 0.5);
    // headshots considered elsewhere
    if (hit_value <= 0.4) {
        bp_hit = bp_torso;
    } else if (one_in(4)) {
        bp_hit = bp_legs;
    } else {
        bp_hit = bp_arms;
    }

    double monster_speed_penalty = std::max(double(get_speed()) / 80., 1.0);
    double goodhit = missed_by / monster_speed_penalty;
    double damage_mult = 1.0;

    if (goodhit <= .1) {
        g->add_msg_if_player(source, _("Headshot!"));
        damage_mult *= rng_float(2.45, 3.35);
        bp_hit = bp_head; // headshot hits the head, of course
    } else if (goodhit <= .2) {
        g->add_msg_if_player(source, _("Critical!"));
        damage_mult *= rng_float(1.75, 2.3);
    } else if (goodhit <= .4) {
        g->add_msg_if_player(source, _("Good hit!"));
        damage_mult *= rng_float(1, 1.5);
    } else if (goodhit <= .6) {
        damage_mult *= rng_float(0.5, 1);
    } else if (goodhit <= .8) {
        g->add_msg_if_player(source, _("Grazing hit."));
        damage_mult *= rng_float(0, .25);
    } else {
        damage_mult *= 0;
    }

    // copy it, since we're mutating
    damage_instance impact = proj.impact;
    if( item(proj.ammo, 0).has_flag("NOGIB") ) {
        impact.add_effect("NOGIB");
    }
    impact.mult_damage(damage_mult);

    dealt_dam = deal_damage(source, bp_hit, side, impact);
    dealt_dam.bp_hit = bp_hit;

    // Apply ammo effects to target.
    const std::string target_material = get_material();
    if (proj.proj_effects.count("FLAME")) {
        if (0 == target_material.compare("veggy") || 0 == target_material.compare("cotton") ||
            0 == target_material.compare("wool") || 0 == target_material.compare("paper") ||
            0 == target_material.compare("wood" ) ) {
            add_effect("onfire", rng(8, 20));
        } else if (0 == target_material.compare("flesh") || 0 == target_material.compare("iflesh") ) {
            add_effect("onfire", rng(5, 10));
        }
    } else if (proj.proj_effects.count("INCENDIARY") ) {
        if (0 == target_material.compare("veggy") || 0 == target_material.compare("cotton") ||
            0 == target_material.compare("wool") || 0 == target_material.compare("paper") ||
            0 == target_material.compare("wood") ) {
            add_effect("onfire", rng(2, 6));
        } else if ( (0 == target_material.compare("flesh") || 0 == target_material.compare("iflesh") ) &&
                    one_in(4) ) {
            add_effect("onfire", rng(1, 4));
        }
    } else if (proj.proj_effects.count("IGNITE")) {
        if (0 == target_material.compare("veggy") || 0 == target_material.compare("cotton") ||
            0 == target_material.compare("wool") || 0 == target_material.compare("paper") ||
            0 == target_material.compare("wood") ) {
            add_effect("onfire", rng(6, 6));
        } else if (0 == target_material.compare("flesh") || 0 == target_material.compare("iflesh") ) {
            add_effect("onfire", rng(10, 10));
        }
    }
    int stun_strength = 0;
    if (proj.proj_effects.count("BEANBAG")) {
        stun_strength = 4;
    }
    if (proj.proj_effects.count("LARGE_BEANBAG")) {
        stun_strength = 16;
    }
    if( stun_strength > 0 ) {
        switch( get_size() ) {
            case MS_TINY:
                stun_strength *= 4;
                break;
            case MS_SMALL:
                stun_strength *= 2;
                break;
            case MS_MEDIUM:
            default:
                break;
            case MS_LARGE:
                stun_strength /= 2;
                break;
            case MS_HUGE:
                stun_strength /= 4;
                break;
        }
        add_effect( "stunned", rng(stun_strength / 2, stun_strength) );
    }

    if(u_see_this) {
        if (damage_mult == 0) {
            if(source != NULL) {
                g->add_msg(source->is_player() ? _("You miss!") : _("The shot misses!"));
            }
        } else if (dealt_dam.total_damage() == 0) {
            g->add_msg(_("The shot reflects off %s %s!"), disp_name(true).c_str(),
                       skin_name().c_str());
        } else if (source != NULL) {
            if (source->is_player()) {
                g->add_msg(_("You hit the %s for %d damage."),
                           disp_name().c_str(), dealt_dam.total_damage());
            } else if( this->is_player() && g->u.has_trait("SELFAWARE")) {
                g->add_msg_if_player( this, _( "You were hit in the %s for %d damage." ),
                                      body_part_name( bp_hit, side ).c_str( ),
                                      dealt_dam.total_damage( ) );
            } else if( u_see_this ) {
                g->add_msg(_("%s shoots %s."),
                           source->disp_name().c_str(), disp_name().c_str());
            }
        }
    }

    return 0;
}

dealt_damage_instance Creature::deal_damage(Creature *source, body_part bp, int side,
                                            const damage_instance &dam)
{
    int total_damage = 0;
    int total_pain = 0;
    damage_instance d = dam; // copy, since we will mutate in absorb_hit

    std::vector<int> dealt_dams(NUM_DT, 0);

    absorb_hit(bp, side, d);

    // add up all the damage units dealt
    int cur_damage;
    for (std::vector<damage_unit>::const_iterator it = d.damage_units.begin();
         it != d.damage_units.end(); ++it) {
        cur_damage = 0;
        deal_damage_handle_type(*it, bp, cur_damage, total_pain);
        if (cur_damage > 0) {
            dealt_dams[it->type] += cur_damage;
            total_damage += cur_damage;
        }
    }

    mod_pain(total_pain);
    if( dam.effects.count("NOGIB") ) {
        total_damage = std::min( total_damage, get_hp() + 1 );
    }

    apply_damage(source, bp, side, total_damage);
    return dealt_damage_instance(dealt_dams);
}
void Creature::deal_damage_handle_type(const damage_unit &du, body_part, int &damage, int &pain)
{
    switch (du.type) {
    case DT_BASH:
        damage += du.amount;
        pain += du.amount / 4; // add up pain before using mod_pain since certain traits modify that
        mod_moves(-rng(0, damage * 2)); // bashing damage reduces moves
        break;
    case DT_CUT:
        damage += du.amount;
        pain += (du.amount + sqrt(double(du.amount))) / 4;
        break;
    case DT_STAB: // stab differs from cut in that it ignores some armor
        damage += du.amount;
        pain += (du.amount + sqrt(double(du.amount))) / 4;
        break;
    case DT_HEAT: // heat damage sets us on fire sometimes
        damage += du.amount;
        pain += du.amount / 4;
        if (rng(0, 100) > (100 - 400 / (du.amount + 3))) {
            add_effect("onfire", rng(1, 3));
        }
        break;
    case DT_ELECTRIC: // electrical damage slows us a lot
        damage += du.amount;
        pain += du.amount / 4;
        mod_moves(-du.amount * 100);
        break;
    case DT_COLD: // cold damage slows us a bit and hurts less
        damage += du.amount;
        pain += du.amount / 6;
        mod_moves(-du.amount * 80);
        break;
    default:
        damage += du.amount;
        pain += du.amount / 4;
    }
}

/*
 * State check functions
 */

bool Creature::is_warm()
{
    return true;
}

bool Creature::is_fake()
{
    return fake;
}

void Creature::set_fake(const bool fake_value)
{
    fake = fake_value;
}

/*
 * Effect-related functions
 */
// Some utility functions for effects
bool is_expired_effect(effect &e)   // utility function for process_effects
{
    if (e.get_duration() <= 0) {
        g->add_msg_string(e.get_effect_type()->get_remove_message());
        g->u.add_memorial_log(pgettext("memorial_male", e.get_effect_type()->get_remove_memorial_log().c_str()),
                              pgettext("memorial_female", e.get_effect_type()->get_remove_memorial_log().c_str()));
        return true;
    } else {
        return false;
    }
}
bool Creature::move_effects()
{
    if (has_effect("beartrap")) {
        mod_moves(-100);
        if(x_in_y(get_str(), 80)) {
            remove_effect("beartrap");
            g->add_msg_if_player(this, _("You free yourself from the bear trap!"));
        } else {
            g->add_msg_if_player(this, _("You try to free yourself from the bear trap, but can't get loose!"));
        }
        return true;
    }
    if (has_effect("downed")) {
        if (rng(0, 40) > get_dex() + int(get_str() / 2)) {
            g->add_msg_if_player(this, _("You struggle to stand."));
            mod_moves(-100);
        } else {
            g->add_msg_if_player(this, _("You stand up."));
            remove_effect("downed");
            mod_moves(-100);
        }
        return true;
    }
    if (has_effect("in_pit")) {
        if (rng(0, 40) > get_str() + int(get_dex() / 2)) {
            g->add_msg_if_player(this, _("You try to escape the pit, but slip back in."));
            mod_moves(-100);
            return true;
        } else {
            g->add_msg_if_player(this, _("You escape the pit!"));
            remove_effect("in_pit");
        }
    }
    return false;
}
void Creature::add_effect(efftype_id eff_id, int dur, bool perm, int intensity, body_part bp,
                            int side)
{
    if (dur <= 0 || intensity <= 0) {
        return;
    }
    if (effect_types[eff_id].main_parts()) {
        if (bp == bp_eyes || bp == bp_mouth) {
            bp = bp_head;
        } else if (bp == bp_hands) {
            bp = bp_arms;
        } else if (bp == bp_feet) {
            bp = bp_legs;
        }
    }

    bool found = false;
    std::vector<effect>::iterator it = effects.begin();
    while ((it != effects.end()) && !found) {
        if (it->get_id() == eff_id) {
            if ((bp == num_bp) ^ (it->get_bp() == num_bp)) {
                debugmsg("Bodypart missmatch when applying effect %s", eff_id.c_str());
                return;
            } else if (it->get_bp() == bp && ((it->get_side() == -1) ^ (side == -1))) {
                debugmsg("Side of body missmatch when applying effect %s", eff_id.c_str());
                return;
            }
            if (it->get_bp() == bp && it->get_side() == side) {
                if (it->get_effect_type()->get_additive() > 0) {
                    it->mod_duration(dur);
                } else if (it->get_effect_type()->get_additive() < 0) {
                    it->mod_duration(-dur);
                    if (it->get_duration() <= 0) {
                        it->set_duration(1);
                    }
                }
                it->mod_intensity(intensity);
                if (it->get_max_intensity() != -1 && it->get_intensity() > it->get_max_intensity()) {
                    it->set_intensity(it->get_max_intensity());
                }
                if (perm) {
                    it->pause_effect();
                }
                found = true;
            }
        }
        ++it;
    }
    if (!found) {
        if (is_player()) { // only print the message if we didn't already have it
            g->add_msg_string(effect_types[eff_id].get_apply_message());
            g->u.add_memorial_log(pgettext("memorial_male",
                                           effect_types[eff_id].get_apply_memorial_log().c_str()),
                                  pgettext("memorial_female",
                                           effect_types[eff_id].get_apply_memorial_log().c_str()));
        }
        effect new_eff(&effect_types[eff_id], dur, perm, intensity, bp, side);
        effects.push_back(new_eff);
    }
    if(is_player()) {
        g->u.recalc_sight_limits();
    }
}
bool Creature::add_env_effect(efftype_id eff_id, body_part vector, int strength, int dur,
                            bool perm, int intensity, body_part bp, int side)
{
    if (dice(strength, 3) > dice(get_env_resist(vector), 3)) {
        add_effect(eff_id, dur, perm, intensity, bp, side);
        return true;
    } else {
        return false;
    }
}
void Creature::clear_effects()
{
    effects.clear();
}
void Creature::remove_effect(efftype_id eff_id, body_part bp, int side)
{
    for (size_t i = 0; i < effects.size();) {
        if (effects[i].get_id() == eff_id && ( bp == num_bp || effects[i].get_bp() == bp ) &&
            ( side == -1 || effects[i].get_side() == side ) ) {
            effects.erase(effects.begin() + i);
            if(is_player()) {
                g->add_msg_string(effects[i].get_effect_type()->get_remove_message());
                g->u.add_memorial_log(pgettext("memorial_male", effects[i].get_effect_type()->get_remove_memorial_log().c_str()),
                                      pgettext("memorial_female", effects[i].get_effect_type()->get_remove_memorial_log().c_str()));
            }
        } else {
            i++;
        }
    }
    if(is_player()) {
        g->u.recalc_sight_limits();
    }
}
bool Creature::has_effect(efftype_id eff_id, body_part bp, int side)
{
    for (std::vector<effect>::iterator it = effects.begin(); it != effects.end(); ++it) {
        if (it->get_id() == eff_id && ( bp == num_bp || it->get_bp() == bp ) &&
            ( side == -1 || it->get_side() == side ) ) {
            return true;
        }
    }
    return false;
}
bool Creature::get_effect(effect &ret, efftype_id eff_id, body_part bp, int side)
{
    for (std::vector<effect>::iterator it = effects.begin(); it != effects.end(); ++it) {
        if (it->get_id() == eff_id && ( bp == num_bp || it->get_bp() == bp ) &&
            ( side == -1 || it->get_side() == side ) ) {
            ret = *it;
            return true;
        }
    }
    return false;
}
int Creature::effect_duration(efftype_id eff_id, bool all, body_part bp, int side)
{
    int tmp = 0;
    for (std::vector<effect>::iterator it = effects.begin(); it != effects.end(); ++it) {
        if (it->get_id() == eff_id && (bp == num_bp || it->get_bp() == bp) &&
            (side == -1 || it->get_side() == side)) {
            if (all == false) {
                return it->get_duration();
            } else {
                tmp += it->get_duration();
            }
        }
    }
    return tmp;
}
int Creature::effect_intensity(efftype_id eff_id, bool all, body_part bp, int side)
{
    int tmp = 0;
    for (std::vector<effect>::iterator it = effects.begin(); it != effects.end(); ++it) {
        if (it->get_id() == eff_id && (bp == num_bp || it->get_bp() == bp) &&
            (side == -1 || it->get_side() == side)) {
            if (all == false) {
                return it->get_intensity();
            } else {
                tmp += it->get_intensity();
            }
        }
    }
    return tmp;
}

void Creature::process_effects()
{
    int health_val = get_healthy();
    for (std::vector<effect>::iterator it = effects.begin(); it != effects.end(); ++it) {
        it->decay(health_val);
    }
    effects.erase(std::remove_if(effects.begin(), effects.end(),
                                 is_expired_effect), effects.end());
}
void Creature::cough(bool harmful)
{
    if (is_player()) {
        g->add_msg(_("You cough heavily."));
        g->sound(xpos(), ypos(), 4, "");
    } else {
        g->sound(xpos(), ypos(), 4, _("a hacking cough."));
    }
    mod_moves(-80);
    if (harmful && !one_in(4)) {
        hurt(bp_torso, -1, 1);
    }
    if (has_effect("sleep") && ((harmful && one_in(3)) || one_in(10)) ) {
        if(is_player()) {
            //wake_up(_("You wake up coughing."));
        }
    }
}
bool Creature::will_vomit(int chance)
{
    bool drunk = g->u.has_disease("drunk");
    bool antiEmetics = g->u.has_disease("weed_high");
    bool hasNausea = g->u.has_trait("NAUSEA") && one_in(chance*2);
    bool stomachUpset = g->u.has_trait("WEAKSTOMACH") && one_in(chance*3);
    bool suppressed = (g->u.has_trait("STRONGSTOMACH") && one_in(2)) ||
        (antiEmetics && !drunk && !one_in(chance));
    return ((stomachUpset || hasNausea) && !suppressed);
}

void Creature::manage_sleep()
{
    // TODO: Still a lot to generalize away from the player here.
    set_moves(0);
    // Hibernating only kicks in whilst Engorged; separate tracking for hunger/thirst here
    // as a safety catch.  One test subject managed to get two Colds during hibernation;
    // since those add fatigue and dry out the character, the subject went for the full 10 days plus
    // a little, and came out of it well into Parched.  Hibernating shouldn't endanger your
    // life like that--but since there's much less fluid reserve than food reserve,
    // simply using the same numbers won't work.
    effect sleep;
    get_effect(sleep, "sleep");
    if((int(g->turn) % 350 == 0) && g->u.has_trait("HIBERNATE") && (g->u.hunger < -60) && !(g->u.thirst >= 80)) {
        int recovery_chance;
        // Hibernators' metabolism slows down: you heal and recover Fatigue much more slowly.
        // Accelerated recovery capped to 2x over 2 hours...well, it was ;-P
        // After 16 hours of activity, equal to 7.25 hours of rest
        if (sleep.get_intensity() < 24) {
            sleep.mod_intensity(1);
        } else if (sleep.get_intensity() < 1) {
            sleep.set_intensity(1);
        }
        recovery_chance = 24 - sleep.get_intensity() + 1;
        if (g->u.fatigue > 0) {
            g->u.fatigue -= 1 + one_in(recovery_chance);
        }
        if ((!(g->u.has_trait("FLIMSY")) && (!(g->u.has_trait("FLIMSY2"))) &&
               (!(g->u.has_trait("FLIMSY3")))) || (g->u.has_trait("FLIMSY") && x_in_y(3 , 4)) ||
               (g->u.has_trait("FLIMSY2") && one_in(2)) ||
               (g->u.has_trait("FLIMSY3") && one_in(4))) {
            int heal_chance = get_healthy() / 25;
            if (g->u.has_trait("FASTHEALER")) {
                heal_chance += 100;
            } else if (g->u.has_trait("FASTHEALER2")) {
                heal_chance += 150;
            } else if (g->u.has_trait("REGEN")) {
                heal_chance += 200;
            } else if (g->u.has_trait("SLOWHEALER")) {
                heal_chance += 13;
            } else {
                heal_chance += 25;
            }
            int tmp_heal = 0;
            while (heal_chance >= 100) {
                tmp_heal++;
                heal_chance -= 100;
            }
            g->u.healall(tmp_heal + x_in_y(heal_chance, 100));
        }

        if (g->u.fatigue <= 0 && g->u.fatigue > -20) {
            g->u.fatigue = -25;
            g->add_msg_if_player(this, _("You feel well rested."));
            sleep.set_duration(dice(3, 100));
            if (is_player()) {
                g->u.add_memorial_log(pgettext("memorial_male", "Awoke from hibernation."),
                                    pgettext("memorial_female", "Awoke from hibernation."));
            }
        }
    // If you hit Very Thirsty, you kick up into regular Sleep as a safety precaution.
    // See above.  No log note for you. :-/
    } else if((int(g->turn) % 50 == 0) && (!(g->u.hunger < -60) || (g->u.thirst >= 80))) {
        int recovery_chance;
        // Accelerated recovery capped to 2x over 2 hours
        // After 16 hours of activity, equal to 7.25 hours of rest
        if (sleep.get_intensity() < 24) {
            sleep.mod_intensity(1);
        } else if (sleep.get_intensity() < 1) {
            sleep.set_intensity(1);
        }
        recovery_chance = 24 - sleep.get_intensity() + 1;
        if (g->u.fatigue > 0) {
            g->u.fatigue -= 1 + one_in(recovery_chance);
            // You fatigue & recover faster with Sleepy
            // Very Sleepy, you just fatigue faster
            if (g->u.has_trait("SLEEPY")) {
                g->u.fatigue -=(1 + one_in(recovery_chance) / 2);
            }
            // Tireless folks recover fatigue really fast
            // as well as gaining it really slowly
            // (Doesn't speed healing any, though...)
            if (g->u.has_trait("WAKEFUL3")) {
                g->u.fatigue -=(2 + one_in(recovery_chance) / 2);
            }
        }
        if ((!(g->u.has_trait("FLIMSY")) && (!(g->u.has_trait("FLIMSY2"))) &&
               (!(g->u.has_trait("FLIMSY3")))) || (g->u.has_trait("FLIMSY") && x_in_y(3 , 4)) ||
               (g->u.has_trait("FLIMSY2") && one_in(2)) ||
               (g->u.has_trait("FLIMSY3") && one_in(4))) {
            int heal_chance = get_healthy() / 25;
            if (g->u.has_trait("FASTHEALER")) {
                heal_chance += 100;
            } else if (g->u.has_trait("FASTHEALER2")) {
                heal_chance += 150;
            } else if (g->u.has_trait("REGEN")) {
                heal_chance += 200;
            } else if (g->u.has_trait("SLOWHEALER")) {
                heal_chance += 13;
            } else {
                heal_chance += 25;
            }
            int tmp_heal = 0;
            while (heal_chance >= 100) {
                tmp_heal++;
                heal_chance -= 100;
            }
            g->u.healall(tmp_heal + x_in_y(heal_chance, 100));

            if (g->u.fatigue <= 0 && g->u.fatigue > -20) {
                g->u.fatigue = -25;
                g->add_msg_if_player(this, _("You feel well rested."));
                sleep.set_duration(dice(3, 100));
            }
        }
    }

    if (int(g->turn) % 100 == 0 && !g->u.has_bionic("bio_recycler") && !(g->u.hunger < -60)) {
        // Hunger and thirst advance more slowly while we sleep. This is the standard rate.
        g->u.hunger--;
        g->u.thirst--;
    }

    // Hunger and thirst advance *much* more slowly whilst we hibernate.
    // (int (g->turn) % 50 would be zero burn.)
    // Very Thirsty catch deliberately NOT applied here, to fend off Dehydration debuffs
    // until the char wakes.  This was time-trial'd quite thoroughly,so kindly don't "rebalance"
    // without a good explanation and taking a night to make sure it works
    // with the extended sleep duration, OK?
    if (int(g->turn) % 70 == 0 && !g->u.has_bionic("bio_recycler") && (g->u.hunger < -60)) {
        g->u.hunger--;
        g->u.thirst--;
    }

    // Check mutation category strengths to see if we're mutated enough to get a dream
    std::string highcat = g->u.get_highest_category();
    int highest = g->u.mutation_category_level[highcat];

    // Determine the strength of effects or dreams based upon category strength
    int strength = 0; // Category too weak for any effect or dream
    if (highest >= 20 && highest < 35) {
        strength = 1; // Low strength
    } else if (highest >= 35 && highest < 50) {
        strength = 2; // Medium strength
    } else if (highest >= 50) {
        strength = 3; // High strength
    }

    // Get a dream if category strength is high enough.
    if (strength != 0) {
        //Once every 6 / 3 / 2 hours, with a bit of randomness
        if ((int(g->turn) % (3600 / strength) == 0) && one_in(3)) {
            // Select a dream
            std::string dream = g->u.get_category_dream(highcat, strength);
            g->add_msg_if_player(this, "%s",dream.c_str());
        }
    }

    int tirednessVal = rng(5, 200) + rng(0,abs(g->u.fatigue * 2 * 5));
    if (g->u.has_trait("HEAVYSLEEPER2") && !g->u.has_trait("HIBERNATE")) {
        // So you can too sleep through noon
        if ((tirednessVal * 1.25) < g->light_level() && (g->u.fatigue < 10 || one_in(g->u.fatigue / 2))) {
            g->add_msg_if_player(this, _("The light wakes you up."));
            sleep.set_duration(1);
        }
        return;}
     // Ursine hibernators would likely do so indoors.  Plants, though, might be in the sun.
    if (g->u.has_trait("HIBERNATE")) {
        if ((tirednessVal * 5) < g->light_level() && (g->u.fatigue < 10 || one_in(g->u.fatigue / 2))) {
            g->add_msg_if_player(this, _("The light wakes you up."));
            sleep.set_duration(1);
        }
        return;}
    if (tirednessVal < g->light_level() && (g->u.fatigue < 10 || one_in(g->u.fatigue / 2))) {
        g->add_msg_if_player(this, _("The light wakes you up."));
        sleep.set_duration(1);
        return;
    }

    // Cold or heat may wake you up.
    // Player will sleep through cold or heat if fatigued enough
    for (int i = 0 ; i < num_bp ; i++) {
        if (g->u.temp_cur[i] < BODYTEMP_VERY_COLD - g->u.fatigue/2) {
            if (one_in(5000)) {
                g->add_msg_if_player(this, _("You toss and turn trying to keep warm."));
            }
            if (g->u.temp_cur[i] < BODYTEMP_FREEZING - g->u.fatigue/2 ||
                                (one_in(g->u.temp_cur[i] + 5000))) {
                g->add_msg_if_player(this, _("The cold wakes you up."));
                sleep.set_duration(1);
                return;
            }
        } else if (g->u.temp_cur[i] > BODYTEMP_VERY_HOT + g->u.fatigue/2) {
            if (one_in(5000)) {
                g->add_msg_if_player(this, _("You toss and turn in the heat."));
            }
            if (g->u.temp_cur[i] > BODYTEMP_SCORCHING + g->u.fatigue/2 ||
                                (one_in(15000 - g->u.temp_cur[i]))) {
                g->add_msg_if_player(this, _("The heat wakes you up."));
                sleep.set_duration(1);
                return;
            }
        }
    }
}

void Creature::mod_pain(int npain)
{
    pain += npain;
}
void Creature::set_pain(int npain)
{
    pain = npain;
}
void Creature::mod_moves(int nmoves)
{
    moves += nmoves;
}
void Creature::set_moves(int nmoves)
{
    moves = nmoves;
}

/*
 * Killer-related things
 */
Creature *Creature::get_killer()
{
    return killer;
}

/*
 * Innate stats getters
 */

// get_stat() always gets total (current) value, NEVER just the base
// get_stat_bonus() is always just the bonus amount
int Creature::get_str() const
{
    return std::max(0, str_max + str_bonus);
}
int Creature::get_dex() const
{
    return std::max(0, dex_max + dex_bonus);
}
int Creature::get_per() const
{
    return std::max(0, per_max + per_bonus);
}
int Creature::get_int() const
{
    return std::max(0, int_max + int_bonus);
}

int Creature::get_str_base() const
{
    return str_max;
}
int Creature::get_dex_base() const
{
    return dex_max;
}
int Creature::get_per_base() const
{
    return per_max;
}
int Creature::get_int_base() const
{
    return int_max;
}



int Creature::get_str_bonus() const
{
    return str_bonus;
}
int Creature::get_dex_bonus() const
{
    return dex_bonus;
}
int Creature::get_per_bonus() const
{
    return per_bonus;
}
int Creature::get_int_bonus() const
{
    return int_bonus;
}

int Creature::get_healthy() const
{
    return healthy;
}
int Creature::get_healthy_mod() const
{
    return healthy_mod;
}

int Creature::get_num_blocks() const
{
    return num_blocks + num_blocks_bonus;
}
int Creature::get_num_dodges() const
{
    return num_dodges + num_dodges_bonus;
}
int Creature::get_num_blocks_bonus() const
{
    return num_blocks_bonus;
}
int Creature::get_num_dodges_bonus() const
{
    return num_dodges_bonus;
}

// currently this is expected to be overridden to actually have use
int Creature::get_env_resist(body_part)
{
    return 0;
}
int Creature::get_armor_bash(body_part)
{
    return armor_bash_bonus;
}
int Creature::get_armor_cut(body_part)
{
    return armor_cut_bonus;
}
int Creature::get_armor_bash_base(body_part)
{
    return armor_bash_bonus;
}
int Creature::get_armor_cut_base(body_part)
{
    return armor_cut_bonus;
}
int Creature::get_armor_bash_bonus()
{
    return armor_bash_bonus;
}
int Creature::get_armor_cut_bonus()
{
    return armor_cut_bonus;
}

int Creature::get_speed()
{
    return get_speed_base() + get_speed_bonus();
}
int Creature::get_dodge()
{
    return get_dodge_base() + get_dodge_bonus();
}
int Creature::get_hit()
{
    return get_hit_base() + get_hit_bonus();
}

int Creature::get_speed_base()
{
    return speed_base;
}
int Creature::get_dodge_base()
{
    return (get_dex() / 2) + int(get_speed() / 150); //Faster = small dodge advantage
}
int Creature::get_hit_base()
{
    return (get_dex() / 2) + 1;
}
int Creature::get_speed_bonus()
{
    return speed_bonus;
}
int Creature::get_dodge_bonus()
{
    return dodge_bonus;
}
int Creature::get_block_bonus()
{
    return block_bonus; //base is 0
}
int Creature::get_hit_bonus()
{
    return hit_bonus; //base is 0
}
int Creature::get_bash_bonus()
{
    return bash_bonus;
}
int Creature::get_cut_bonus()
{
    return cut_bonus;
}

float Creature::get_bash_mult()
{
    return bash_mult;
}
float Creature::get_cut_mult()
{
    return cut_mult;
}

bool Creature::get_melee_quiet()
{
    return melee_quiet;
}
int Creature::get_grab_resist()
{
    return grab_resist;
}
int Creature::get_throw_resist()
{
    return throw_resist;
}

/*
 * Innate stats setters
 */

void Creature::set_str_bonus(int nstr)
{
    str_bonus = nstr;
}
void Creature::set_dex_bonus(int ndex)
{
    dex_bonus = ndex;
}
void Creature::set_per_bonus(int nper)
{
    per_bonus = nper;
}
void Creature::set_int_bonus(int nint)
{
    int_bonus = nint;
}
void Creature::mod_str_bonus(int nstr)
{
    str_bonus += nstr;
}
void Creature::mod_dex_bonus(int ndex)
{
    dex_bonus += ndex;
}
void Creature::mod_per_bonus(int nper)
{
    per_bonus += nper;
}
void Creature::mod_int_bonus(int nint)
{
    int_bonus += nint;
}


void Creature::set_healthy(int nhealthy)
{
    healthy = nhealthy;
}
void Creature::set_healthy_mod(int nhealthy_mod)
{
    healthy_mod = nhealthy_mod;
}
void Creature::mod_healthy(int nhealthy)
{
    healthy += nhealthy;
}
void Creature::mod_healthy_mod(int nhealthy_mod)
{
    healthy_mod += nhealthy_mod;
}

void Creature::set_num_blocks_bonus(int nblocks)
{
    num_blocks_bonus = nblocks;
}
void Creature::set_num_dodges_bonus(int ndodges)
{
    num_dodges_bonus = ndodges;
}

void Creature::set_armor_bash_bonus(int nbasharm)
{
    armor_bash_bonus = nbasharm;
}
void Creature::set_armor_cut_bonus(int ncutarm)
{
    armor_cut_bonus = ncutarm;
}

void Creature::set_speed_base(int nspeed)
{
    speed_base = nspeed;
}
void Creature::set_speed_bonus(int nspeed)
{
    speed_bonus = nspeed;
}
void Creature::set_dodge_bonus(int ndodge)
{
    dodge_bonus = ndodge;
}
void Creature::set_block_bonus(int nblock)
{
    block_bonus = nblock;
}
void Creature::set_hit_bonus(int nhit)
{
    hit_bonus = nhit;
}
void Creature::set_bash_bonus(int nbash)
{
    bash_bonus = nbash;
}
void Creature::set_cut_bonus(int ncut)
{
    cut_bonus = ncut;
}
void Creature::mod_speed_bonus(int nspeed)
{
    speed_bonus += nspeed;
}
void Creature::mod_dodge_bonus(int ndodge)
{
    dodge_bonus += ndodge;
}
void Creature::mod_block_bonus(int nblock)
{
    block_bonus += nblock;
}
void Creature::mod_hit_bonus(int nhit)
{
    hit_bonus += nhit;
}
void Creature::mod_bash_bonus(int nbash)
{
    bash_bonus += nbash;
}
void Creature::mod_cut_bonus(int ncut)
{
    cut_bonus += ncut;
}

void Creature::set_bash_mult(float nbashmult)
{
    bash_mult = nbashmult;
}
void Creature::set_cut_mult(float ncutmult)
{
    cut_mult = ncutmult;
}

void Creature::set_melee_quiet(bool nquiet)
{
    melee_quiet = nquiet;
}
void Creature::set_grab_resist(int ngrabres)
{
    grab_resist = ngrabres;
}
void Creature::set_throw_resist(int nthrowres)
{
    throw_resist = nthrowres;
}

/*
 * Event handlers
 */

void Creature::on_gethit(Creature *, body_part, damage_instance &)
{
    // does nothing by default
}

/*
 * Drawing-related functions
 */
void Creature::draw(WINDOW *w, int player_x, int player_y, bool inverted)
{
    int draw_x = getmaxx(w) / 2 + xpos() - player_x;
    int draw_y = getmaxy(w) / 2 + ypos() - player_y;
    if(inverted) {
        mvwputch_inv(w, draw_y, draw_x, basic_symbol_color(), symbol());
    } else if(is_symbol_highlighted()) {
        mvwputch_hi(w, draw_y, draw_x, basic_symbol_color(), symbol());
    } else {
        mvwputch(w, draw_y, draw_x, symbol_color(), symbol() );
    }
}

nc_color Creature::basic_symbol_color()
{
    return c_ltred;
}

nc_color Creature::symbol_color()
{
    return basic_symbol_color();
}

bool Creature::is_symbol_highlighted()
{
    return false;
}

char Creature::symbol()
{
    return '?';
}

body_part Creature::select_body_part(Creature *source, int hit_roll)
{
    // Get size difference (-1,0,1);
    int szdif = source->get_size() - get_size();
    if(szdif < -1) {
        szdif = -1;
    } else if (szdif > 1) {
        szdif = 1;
    }

    if(g->debugmon) {
        g->add_msg("source size = %d", source->get_size());
        g->add_msg("target size = %d", get_size());
        g->add_msg("difference = %d", szdif);
    }

    std::map<body_part, double> hit_weights = default_hit_weights[szdif];
    std::map<body_part, double>::iterator iter;

    // If the target is on the ground, even small/tiny creatures may target eyes/head. Also increases chances of larger creatures.
    // Any hit modifiers to locations should go here. (Tags, attack style, etc)
    if(is_on_ground()) {
        hit_weights[bp_eyes] += 10;
        hit_weights[bp_head] += 20;
    }

    //Adjust based on hit roll: Eyes, Head & Torso get higher, while Arms and Legs get lower.
    //This should eventually be replaced with targeted attacks and this being miss chances.
    hit_weights[bp_eyes] = floor(hit_weights[bp_eyes] * pow(hit_roll, 1.15) * 10);
    hit_weights[bp_head] = floor(hit_weights[bp_head] * pow(hit_roll, 1.15) * 10);
    hit_weights[bp_torso] = floor(hit_weights[bp_torso] * pow(hit_roll, 1) * 10);
    hit_weights[bp_arms] = floor(hit_weights[bp_arms] * pow(hit_roll, 0.95) * 10);
    hit_weights[bp_legs] = floor(hit_weights[bp_legs] * pow(hit_roll, 0.975) * 10);


    // Debug for seeing weights.
    if(g->debugmon) {
        g->add_msg("eyes = %f", hit_weights.at(bp_eyes));
        g->add_msg("head = %f", hit_weights.at(bp_head));
        g->add_msg("torso = %f", hit_weights.at(bp_torso));
        g->add_msg("arms = %f", hit_weights.at(bp_arms));
        g->add_msg("legs = %f", hit_weights.at(bp_legs));
    }

    double totalWeight = 0;
    std::set<weight_pair, weight_compare> adjusted_weights;
    for(iter = hit_weights.begin(); iter != hit_weights.end(); ++iter) {
        totalWeight += iter->second;
        adjusted_weights.insert(*iter);
    }

    double roll = rng_float(1, totalWeight);
    body_part selected_part = bp_torso;

    std::set<weight_pair, weight_compare>::iterator adj_iter;
    for(adj_iter = adjusted_weights.begin(); adj_iter != adjusted_weights.end(); ++adj_iter) {
        roll -= adj_iter->second;
        if(roll <= 0) {
            selected_part = adj_iter->first;
            break;
        }
    }

    return selected_part;
}

void Creature::init_hit_weights()
{
    std::map<body_part, double> attacker_equal_weights;
    std::map<body_part, double> attacker_smaller_weights;
    std::map<body_part, double> attacker_bigger_weights;

    attacker_equal_weights[bp_eyes] = 10.f;
    attacker_equal_weights[bp_head] = 20.f;
    attacker_equal_weights[bp_torso] = 55.f;
    attacker_equal_weights[bp_arms] = 55.f;
    attacker_equal_weights[bp_legs] = 35.f;

    attacker_smaller_weights[bp_eyes] = 0.f;
    attacker_smaller_weights[bp_head] = 0.f;
    attacker_smaller_weights[bp_torso] = 55.f;
    attacker_smaller_weights[bp_arms] = 35.f;
    attacker_smaller_weights[bp_legs] = 55.f;

    attacker_bigger_weights[bp_eyes] = 5.f;
    attacker_bigger_weights[bp_head] = 25.f;
    attacker_bigger_weights[bp_torso] = 55.f;
    attacker_bigger_weights[bp_arms] = 55.f;
    attacker_bigger_weights[bp_legs] = 20.f;

    default_hit_weights[-1] = attacker_smaller_weights;
    default_hit_weights[0] = attacker_equal_weights;
    default_hit_weights[1] = attacker_bigger_weights;
}

Creature& Creature::operator= (const Creature& rhs)
{
    str_cur = rhs.str_cur;
    dex_cur = rhs.dex_cur;
    int_cur = rhs.int_cur;
    per_cur = rhs.per_cur;

    str_max = rhs.str_max;
    dex_max = rhs.dex_max;
    int_max = rhs.int_max;
    per_max = rhs.per_max;

    moves = rhs.moves;
    pain = rhs.pain;

    killer = rhs.killer;
    effects = rhs.effects;

    str_bonus = rhs.str_bonus;
    dex_bonus = rhs.dex_bonus;
    per_bonus = rhs.per_bonus;
    int_bonus = rhs.int_bonus;

    num_blocks = rhs.num_blocks;
    num_dodges = rhs.num_dodges;
    num_blocks_bonus = rhs.num_blocks_bonus;
    num_dodges_bonus = rhs.num_dodges_bonus;

    armor_bash_bonus = rhs.armor_bash_bonus;
    armor_cut_bonus = rhs.armor_cut_bonus;

    speed_base = rhs.speed_base;

    speed_bonus = rhs.speed_bonus;
    dodge_bonus = rhs.dodge_bonus;
    block_bonus = rhs.block_bonus;
    hit_bonus = rhs.hit_bonus;
    bash_bonus = rhs.bash_bonus;
    cut_bonus = rhs.cut_bonus;

    bash_mult = rhs.bash_mult;
    cut_mult = rhs.cut_mult;
    melee_quiet = rhs.melee_quiet;

    grab_resist = rhs.grab_resist;
    throw_resist = rhs.throw_resist;

    fake = rhs.fake;
    return *this;
}
