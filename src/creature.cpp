#include "item.h"
#include "creature.h"
#include "output.h"
#include "game.h"
#include "messages.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <map>

static std::map<int, std::map<body_part, double> > default_hit_weights = {
    {
        -1, /* attacker smaller */
        {   { bp_eyes, 0.f },
            { bp_head, 0.f },
            { bp_torso, 55.f },
            { bp_arm_l, 18.f },
            { bp_arm_r, 18.f },
            { bp_leg_l, 28.f },
            { bp_leg_r, 28.f }
        }
    },
    {
        0, /* attacker equal size */
        {   { bp_eyes, 10.f },
            { bp_head, 20.f },
            { bp_torso, 55.f },
            { bp_arm_l, 28.f },
            { bp_arm_r, 28.f },
            { bp_leg_l, 18.f },
            { bp_leg_r, 18.f }
        }
    },
    {
        1, /* attacker larger */
        {   { bp_eyes, 5.f },
            { bp_head, 25.f },
            { bp_torso, 55.f },
            { bp_arm_l, 28.f },
            { bp_arm_r, 28.f },
            { bp_leg_l, 10.f },
            { bp_leg_r, 10.f }
        }
    }
};

struct weight_compare {
    bool operator() (const std::pair<body_part, double> &left,
                     const std::pair<body_part, double> &right)
    {
        return left.second < right.second;
    }
};

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
}

void Creature::process_turn()
{
    process_effects();

    // Call this in case any effects have changed our stats
    reset_stats();

    // add an appropriate number of moves
    moves += get_speed();
}

// MF_DIGS or MF_CAN_DIG and diggable terrain
bool Creature::digging() const
{
    return false;
}

/*
 * Damage-related functions
 */

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
    block_hit(source, bp_hit, d);

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
        if (is_player() && (!g->u.has_trait("LEG_TENT_BRACE") || g->u.footwear_factor() == 1 ||
                            (g->u.footwear_factor() == .5 && one_in(2))) ) {
            // can the player force their self to the ground? probably not.
            source->add_msg_if_npc( m_bad, _("<npcname> forces you to the ground!"));
        } else {
            source->add_msg_player_or_npc( m_good, _("You force %s to the ground!"),
                                           _("<npcname> forces %s to the ground!"),
                                           disp_name().c_str() );
        }
        if (!g->u.has_trait("LEG_TENT_BRACE") || g->u.footwear_factor() == 1 ||
            (g->u.footwear_factor() == .5 && one_in(2))) {
            add_effect("downed", 1);
            mod_moves(-stab_moves / 2);
        }
    } else {
        mod_moves(-stab_moves);
    }

    on_gethit(source, bp_hit, d); // trigger on-gethit events
    dealt_dam = deal_damage(source, bp_hit, d);
    dealt_dam.bp_hit = bp_hit;
}

// TODO: check this over, see if it's right
/**
 * Attempts to harm a creature with a projectile.
 *
 * @param source Pointer to the creature who shot the projectile.
 * @param missed_by Deviation of the projectile.
 * @param proj Reference to the projectile hitting the creature.
 * @param dealt_dam A reference storing the damage dealt.
 * @return 0 signals that the projectile should stop,
 *         1 signals that the projectile should not stop (i.e. dodged, passed through).
 */
int Creature::deal_projectile_attack(Creature *source, double missed_by,
                                     const projectile &proj, dealt_damage_instance &dealt_dam)
{
    bool u_see_this = g->u_see(this);
    body_part bp_hit;

    // do 10,speed because speed could potentially be > 10000
    if (dodge_roll() >= dice(10, proj.speed)) {
        if (is_player())
            add_msg(_("You dodge %s projectile!"),
                    source->disp_name(true).c_str());
        else if (u_see_this)
            add_msg(_("%s dodges %s projectile."),
                    disp_name().c_str(), source->disp_name(true).c_str());
        return 1;
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
        if( one_in(2)) {
            bp_hit = bp_leg_l;
        } else {
            bp_hit = bp_leg_r;
        }
    } else {
        if( one_in(2)) {
            bp_hit = bp_arm_l;
        } else {
            bp_hit = bp_arm_r;
        }
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

    dealt_dam = deal_damage(source, bp_hit, impact);
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
                            //~ “hit points”, used in scrolling combat text
                            _("hp"), m_neutral,
                            "hp");
                } else {
                    SCT.removeCreatureHP();
                }

                add_msg(m_good, _("You hit the %s for %d damage."),
                        disp_name().c_str(), dealt_dam.total_damage());

            } else if(this->is_player()) {
                //monster hits player ranged
                //~ Hit message. 1$s is bodypart name in accusative. 2$d is damage value.
                add_msg_if_player(m_bad, _( "You were hit in the %1$s for %2$d damage." ),
                                  body_part_name_accusative(bp_hit).c_str( ),
                                  dealt_dam.total_damage());
            } else if( u_see_this ) {
                add_msg(_("%s shoots %s."),
                        source->disp_name().c_str(), disp_name().c_str());
            }
        }
    }
    if (this->is_dead_state()) {
        this->die(source);
    }
    return 0;
}

dealt_damage_instance Creature::deal_damage(Creature *source, body_part bp,
        const damage_instance &dam)
{
    int total_damage = 0;
    int total_pain = 0;
    damage_instance d = dam; // copy, since we will mutate in absorb_hit

    std::vector<int> dealt_dams(NUM_DT, 0);

    absorb_hit(bp, d);

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

    apply_damage(source, bp, total_damage);
    if( is_dead_state() ) {
        die( source );
    }
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

bool Creature::is_warm() const
{
    return true;
}

bool Creature::is_fake() const
{
    return fake;
}

void Creature::set_fake(const bool fake_value)
{
    fake = fake_value;
}

/*
 * Effect-related methods
 */
void Creature::add_effect(efftype_id eff_id, int dur, int intensity, bool permanent)
{
    // check if we already have it
    auto found_effect = effects.find( eff_id );

    if (found_effect != effects.end()) {
        effect &e = found_effect->second;
        // if we do, mod the duration
        e.mod_duration(dur);
        // Adding a permanent effect makes it permanent
        if( e.is_permanent() ) {
            e.pause_effect();
        }
        if( e.get_intensity() + intensity <= e.get_max_intensity() ) {
            e.mod_intensity( intensity );
        }
    } else {
        // if we don't already have it then add a new one
        if (effect_types.find(eff_id) == effect_types.end()) {
            return;
        }
        effect new_eff(&effect_types[eff_id], dur, intensity, permanent);
        effects[eff_id] = new_eff;
        if (is_player()) { // only print the message if we didn't already have it
            if(effect_types[eff_id].get_apply_message() != "") {
                     add_msg(effect_types[eff_id].gain_game_message_type(),
                             _(effect_types[eff_id].get_apply_message().c_str()));
            }
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
    effects.erase( rem_id );
}
bool Creature::has_effect(efftype_id eff_id) const
{
    // return if any effect in effects has this id
    return effects.find( eff_id ) != effects.end();
}
void Creature::process_effects()
{
    for( auto it = effects.begin(); it != effects.end(); ++it ) {
        if( !it->second.is_permanent() ) {
            it->second.mod_duration( -1 );
            if (g->debugmon) {
                debugmsg("Duration %d", it->second.get_duration());
            }
        }
    }
    for( auto it = effects.begin(); it != effects.end(); ) {
        if( !it->second.is_permanent() && it->second.get_duration() <= 0 ) {
            const effect_type *type = it->second.get_effect_type();
            if(type->get_remove_message() != "") {
                add_msg( type->lose_game_message_type(), _(type->get_remove_message().c_str()) );
            }
            g->u.add_memorial_log(
                pgettext("memorial_male", type->get_remove_memorial_log().c_str() ),
                pgettext("memorial_female", type->get_remove_memorial_log().c_str()) );
            const auto id = it->second.get_id();
            ++it;
            remove_effect( id );
        } else {
            ++it;
        }
    }
}

void Creature::update_health(int base_threshold)
{
    if (get_healthy_mod() > 200) {
        set_healthy_mod(200);
    } else if (get_healthy_mod() < -200) {
        set_healthy_mod(-200);
    }
    int roll = rng(-100, 100);
    base_threshold += get_healthy() - get_healthy_mod();
    if (roll > base_threshold) {
        mod_healthy(1);
    } else if (roll < base_threshold) {
        mod_healthy(-1);
    }
    set_healthy_mod(get_healthy_mod() * 3 / 4);

    if (g->debugmon) {
        debugmsg("Health: %d, Health mod: %d", get_healthy(), get_healthy_mod());
    }
}

// Methods for setting/getting misc key/value pairs.
void Creature::set_value( const std::string key, const std::string value )
{
    values[ key ] = value;
}

void Creature::remove_value( const std::string key )
{
    values.erase( key );
}

std::string Creature::get_value( const std::string key ) const
{
    auto it = values.find( key );
    return ( it == values.end() ) ? "" : it->second;
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
Creature *Creature::get_killer() const
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
int Creature::get_env_resist(body_part) const
{
    return 0;
}
int Creature::get_armor_bash(body_part) const
{
    return armor_bash_bonus;
}
int Creature::get_armor_cut(body_part) const
{
    return armor_cut_bonus;
}
int Creature::get_armor_bash_base(body_part) const
{
    return armor_bash_bonus;
}
int Creature::get_armor_cut_base(body_part) const
{
    return armor_cut_bonus;
}
int Creature::get_armor_bash_bonus() const
{
    return armor_bash_bonus;
}
int Creature::get_armor_cut_bonus() const
{
    return armor_cut_bonus;
}

int Creature::get_speed() const
{
    return get_speed_base() + get_speed_bonus();
}
int Creature::get_dodge() const
{
    return get_dodge_base() + get_dodge_bonus();
}
int Creature::get_melee() const
{
    return 0;
}
int Creature::get_hit() const
{
    return get_hit_base() + get_hit_bonus();
}

int Creature::get_speed_base() const
{
    return speed_base;
}
int Creature::get_dodge_base() const
{
    return (get_dex() / 2) + int(get_speed() / 150); //Faster = small dodge advantage
}
int Creature::get_hit_base() const
{
    return (get_dex() / 2) + 1;
}
int Creature::get_speed_bonus() const
{
    return speed_bonus;
}
int Creature::get_dodge_bonus() const
{
    return dodge_bonus;
}
int Creature::get_block_bonus() const
{
    return block_bonus; //base is 0
}
int Creature::get_hit_bonus() const
{
    return hit_bonus; //base is 0
}
int Creature::get_bash_bonus() const
{
    return bash_bonus;
}
int Creature::get_cut_bonus() const
{
    return cut_bonus;
}

float Creature::get_bash_mult() const
{
    return bash_mult;
}
float Creature::get_cut_mult() const
{
    return cut_mult;
}

bool Creature::get_melee_quiet() const
{
    return melee_quiet;
}
int Creature::get_grab_resist() const
{
    return grab_resist;
}
int Creature::get_throw_resist() const
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
void Creature::mod_healthy(int nhealthy)
{
    healthy += nhealthy;
}
void Creature::set_healthy_mod(int nhealthy_mod)
{
    healthy_mod = nhealthy_mod;
}
void Creature::mod_healthy_mod(int nhealthy_mod)
{
    healthy_mod += nhealthy_mod;
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
    } else if( stat == "healthy" ) {
        mod_healthy( modifier );
    } else if( stat == "healthy_mod" ) {
        mod_healthy_mod( modifier );
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

int Creature::weight_capacity() const
{
    int base_carry = 13000 + get_str() * 4000;
    switch( get_size() ) {
    case MS_TINY:
        base_carry /= 4;
        break;
    case MS_SMALL:
        base_carry /= 2;
        break;
    case MS_MEDIUM:
    default:
        break;
    case MS_LARGE:
        base_carry *= 2;
        break;
    case MS_HUGE:
        base_carry *= 4;
        break;
    }

    return base_carry;
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
void Creature::draw(WINDOW *w, int player_x, int player_y, bool inverted) const
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

nc_color Creature::basic_symbol_color() const
{
    return c_ltred;
}

nc_color Creature::symbol_color() const
{
    return basic_symbol_color();
}

bool Creature::is_symbol_highlighted() const
{
    return false;
}

const std::string &Creature::symbol() const
{
    static const std::string default_symbol("?");
    return default_symbol;
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
    hit_weights[bp_arm_l] = floor(hit_weights[bp_arm_l] * std::pow(hit_roll, 0.95) * 10);
    hit_weights[bp_arm_r] = floor(hit_weights[bp_arm_r] * std::pow(hit_roll, 0.95) * 10);
    hit_weights[bp_leg_l] = floor(hit_weights[bp_leg_l] * std::pow(hit_roll, 0.975) * 10);
    hit_weights[bp_leg_r] = floor(hit_weights[bp_leg_r] * std::pow(hit_roll, 0.975) * 10);


    // Debug for seeing weights.
    if(g->debugmon) {
        add_msg(m_info, "eyes = %f", hit_weights.at(bp_eyes));
        add_msg(m_info, "head = %f", hit_weights.at(bp_head));
        add_msg(m_info, "torso = %f", hit_weights.at(bp_torso));
        add_msg(m_info, "arm_l = %f", hit_weights.at(bp_arm_l));
        add_msg(m_info, "arm_r = %f", hit_weights.at(bp_arm_r));
        add_msg(m_info, "leg_l = %f", hit_weights.at(bp_leg_l));
        add_msg(m_info, "leg_r = %f", hit_weights.at(bp_leg_r));
    }

    double totalWeight = 0;
    std::set<std::pair<body_part, double>, weight_compare> adjusted_weights;
    for(iter = hit_weights.begin(); iter != hit_weights.end(); ++iter) {
        totalWeight += iter->second;
        adjusted_weights.insert(*iter);
    }

    double roll = rng_float(1, totalWeight);
    body_part selected_part = bp_torso;

    std::set<std::pair<body_part, double>, weight_compare>::iterator adj_iter;
    for(adj_iter = adjusted_weights.begin(); adj_iter != adjusted_weights.end(); ++adj_iter) {
        roll -= adj_iter->second;
        if(roll <= 0) {
            selected_part = adj_iter->first;
            break;
        }
    }

    return selected_part;
}

Creature &Creature::operator= (const Creature &rhs)
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
