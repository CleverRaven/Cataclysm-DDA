#include "item.h"
#include "creature.h"
#include "output.h"
#include "game.h"
#include "messages.h"
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
            source->add_msg_if_npc( m_bad, _("<npcname> forces you to the ground!"));
        } else {
            source->add_msg_player_or_npc( m_good, _("You force %s to the ground!"),
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
            add_msg(_("You dodge %s projectile!"),
                       source->disp_name(true).c_str());
        else if (u_see_this)
            add_msg(_("%s dodges %s projectile."),
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

    std::string message = "";
    game_message_type gmtSCTcolor = m_neutral;

    if (goodhit <= .1) {
        message = _("Headshot!");
        source->add_msg_if_player(m_good, message.c_str());
        gmtSCTcolor = m_headshot;
        damage_mult *= rng_float(2.45, 3.35);
        bp_hit = bp_head; // headshot hits the head, of course
    } else if (goodhit <= .2) {
        message = _("Critical!");
        source->add_msg_if_player(m_good, message.c_str());
        gmtSCTcolor = m_critical;
        damage_mult *= rng_float(1.75, 2.3);
    } else if (goodhit <= .4) {
        message = _("Good hit!");
        source->add_msg_if_player(m_good, message.c_str());
        gmtSCTcolor = m_good;
        damage_mult *= rng_float(1, 1.5);
    } else if (goodhit <= .6) {
        damage_mult *= rng_float(0.5, 1);
    } else if (goodhit <= .8) {
        message = _("Grazing hit.");
        source->add_msg_if_player(m_good, message.c_str());
        gmtSCTcolor = m_grazing;
        damage_mult *= rng_float(0, .25);
    } else {
        damage_mult *= 0;
    }

    // copy it, since we're mutating
    damage_instance impact = proj.impact;
    if( item(proj.ammo->id, 0).has_flag("NOGIB") ) {
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
    if(proj.proj_effects.count("WHIP")) {
        stun_strength = rng(4, 10);
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
                add_msg(source->is_player() ? _("You miss!") : _("The shot misses!"));
            }
        } else if (dealt_dam.total_damage() == 0) {
            add_msg(_("The shot reflects off %s %s!"), disp_name(true).c_str(),
                       skin_name().c_str());
        } else if (source != NULL) {
            if (source->is_player()) {
                //player hits monster ranged
                nc_color color;
                std::string health_bar = "";
                get_HP_Bar(dealt_dam.total_damage(), this->get_hp_max(), color, health_bar, true);

                SCT.add(this->xpos(),
                        this->ypos(),
                        direction_from(0, 0, this->xpos() - source->xpos(), this->ypos() - source->ypos()),
                        health_bar, m_good,
                        message, gmtSCTcolor);

                if (this->get_hp() > 0) {
                    get_HP_Bar(this->get_hp(), this->get_hp_max(), color, health_bar, true);

                    SCT.add(this->xpos(),
                            this->ypos(),
                            direction_from(0, 0, this->xpos() - source->xpos(), this->ypos() - source->ypos()),
                            health_bar, m_good,
                            "hp", m_neutral,
                            "hp");
                } else {
                    SCT.removeCreatureHP();
                }

                add_msg(m_good, _("You hit the %s for %d damage."),
                           disp_name().c_str(), dealt_dam.total_damage());

            } else if(this->is_player()) {
                //monster hits player ranged
                add_msg_if_player( m_bad, _( "You were hit in the %s for %d damage." ),
                                          body_part_name( bp_hit, side ).c_str( ),
                                          dealt_dam.total_damage( ) );
            } else if( u_see_this ) {
                add_msg(_("%s shoots %s."),
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
class is_id_functor   // functor for remove/has_effect, give c++11 lambdas pls
{
        std::string id;
    public:
        is_id_functor(efftype_id rem_id) : id(rem_id) {}
        bool operator() (effect &e) {
            return e.get_id() == id;
        }
};
bool is_expired_effect(effect &e)   // utility function for process_effects
{
    if (e.get_duration() <= 0) {
        add_msg(e.get_effect_type()->lose_game_message_type(), e.get_effect_type()->get_remove_message().c_str());
        g->u.add_memorial_log(pgettext("memorial_male", e.get_effect_type()->get_remove_memorial_log().c_str()),
                              pgettext("memorial_female", e.get_effect_type()->get_remove_memorial_log().c_str()));
        return true;
    } else {
        return false;
    }
}

void Creature::add_effect(efftype_id eff_id, int dur, int intensity, bool permanent)
{
    // check if we already have it
    std::vector<effect>::iterator first_eff =
        std::find_if(effects.begin(), effects.end(), is_id_functor(eff_id));

    if (first_eff != effects.end()) {
        // if we do, mod the duration
        first_eff->mod_duration(dur);
        // Adding a permanent effect makes it permanent
        if (first_eff->is_permanent()) {
            first_eff->pause_effect();
        }
        if (first_eff->get_intensity() + intensity <= first_eff->get_max_intensity()) {
            first_eff->mod_intensity(intensity);
        }
    } else {
        // if we don't already have it then add a new one
        if (effect_types.find(eff_id) == effect_types.end()) {
            return;
        }
        effect new_eff(&effect_types[eff_id], dur, intensity, permanent);
        effects.push_back(new_eff);
        if (is_player()) { // only print the message if we didn't already have it
            add_msg(effect_types[eff_id].gain_game_message_type(), effect_types[eff_id].get_apply_message().c_str());
            g->u.add_memorial_log(pgettext("memorial_male",
                                           effect_types[eff_id].get_apply_memorial_log().c_str()),
                                  pgettext("memorial_female",
                                           effect_types[eff_id].get_apply_memorial_log().c_str()));
        }
    }
}
bool Creature::add_env_effect(efftype_id eff_id, body_part vector, int strength, int dur,
                                int intensity, bool permanent)
{
    if (dice(strength, 3) > dice(get_env_resist(vector), 3)) {
        add_effect(eff_id, dur, intensity, permanent);
        return true;
    } else {
        return false;
    }
}
void Creature::clear_effects()
{
    effects.clear();
}
void Creature::remove_effect(efftype_id rem_id)
{
    // remove all effects with this id
    effects.erase(std::remove_if(effects.begin(), effects.end(),
                                 is_id_functor(rem_id)), effects.end());
}
bool Creature::has_effect(efftype_id eff_id)
{
    // return if any effect in effects has this id
    return (std::find_if(effects.begin(), effects.end(), is_id_functor(eff_id)) !=
            effects.end());
}
void Creature::process_effects()
{
    for (std::vector<effect>::iterator it = effects.begin();
         it != effects.end(); ++it) {
        if (!it->is_permanent() && it->get_duration() > 0) {
            it->mod_duration(-1);
            if (g->debugmon) {
                debugmsg("Duration %d", it->get_duration());
            }
        }
    }
    effects.erase(std::remove_if(effects.begin(), effects.end(),
                                 is_expired_effect), effects.end());
}


void Creature::mod_pain(int npain)
{
    pain += npain;
}
void Creature::mod_moves(int nmoves)
{
    moves += nmoves;
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

int Creature::get_speed() const
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

int Creature::get_speed_base() const
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
int Creature::get_speed_bonus() const
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

void Creature::mod_stat( std::string stat, int modifier )
{
    if( stat == "str" ) {
        mod_str_bonus( modifier );
    } else if( stat == "dex" ) {
        mod_dex_bonus( modifier );
    } else if( stat == "per" ) {
        mod_per_bonus( modifier );
    } else if( stat == "int" ) {
        mod_int_bonus( modifier );
    } else if( stat == "speed" ) {
        mod_speed_bonus( modifier );
    } else if( stat == "dodge" ) {
        mod_dodge_bonus( modifier );
    } else if( stat == "block" ) {
        mod_block_bonus( modifier );
    } else if( stat == "hit" ) {
        mod_hit_bonus( modifier );
    } else if( stat == "bash" ) {
        mod_bash_bonus( modifier );
    } else if( stat == "cut" ) {
        mod_cut_bonus( modifier );
    } else if( stat == "pain" ) {
        mod_pain( modifier );
    } else if( stat == "moves" ) {
        mod_moves( modifier );
    } else {
        add_msg( "Tried to modify a nonexistent stat %s.", stat.c_str() );
    }
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
        add_msg(m_info, "source size = %d", source->get_size());
        add_msg(m_info, "target size = %d", get_size());
        add_msg(m_info, "difference = %d", szdif);
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
    hit_weights[bp_eyes] = floor(hit_weights[bp_eyes] * std::pow(hit_roll, 1.15) * 10);
    hit_weights[bp_head] = floor(hit_weights[bp_head] * std::pow(hit_roll, 1.15) * 10);
    hit_weights[bp_torso] = floor(hit_weights[bp_torso] * std::pow(hit_roll, 1) * 10);
    hit_weights[bp_arms] = floor(hit_weights[bp_arms] * std::pow(hit_roll, 0.95) * 10);
    hit_weights[bp_legs] = floor(hit_weights[bp_legs] * std::pow(hit_roll, 0.975) * 10);


    // Debug for seeing weights.
    if(g->debugmon) {
        add_msg(m_info, "eyes = %f", hit_weights.at(bp_eyes));
        add_msg(m_info, "head = %f", hit_weights.at(bp_head));
        add_msg(m_info, "torso = %f", hit_weights.at(bp_torso));
        add_msg(m_info, "arms = %f", hit_weights.at(bp_arms));
        add_msg(m_info, "legs = %f", hit_weights.at(bp_legs));
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
