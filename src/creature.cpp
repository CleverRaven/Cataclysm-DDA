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

Creature::~Creature()
{
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
    if(is_dead_state()) {
        return;
    }
    reset_bonuses();

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

bool Creature::sees( const Creature &critter, int &bresenham_slope ) const
{
    int cx = critter.xpos();
    int cy = critter.ypos();
    const int wanted_range = rl_dist( pos(), critter.pos() );
    if( ( wanted_range > 1 && critter.digging() ) ||
        ( g->m.is_divable( cx, cy ) && critter.is_underwater() && !is_underwater() ) ) {
        return false;
    }

    return sees( cx, cy, bresenham_slope );
}

bool Creature::sees( const int tx, const int ty, int &bresenham_slope ) const
{
    const int range_min = sight_range( g->light_level() );
    const int range_max = sight_range( DAYLIGHT_LEVEL );
    const int wanted_range = rl_dist( xpos(), ypos(), tx, ty );
    if( wanted_range <= range_min ||
        ( wanted_range <= range_max &&
          g->m.light_at( tx, ty ) >= LL_LOW ) ) {
        if( is_player() ) {
            return g->m.pl_sees( xpos(), ypos(), tx, ty, wanted_range);
        } else if( g->m.light_at( tx, ty ) >= LL_LOW ) {
            return g->m.sees( xpos(), ypos(), tx, ty, wanted_range, bresenham_slope );
        } else {
            return g->m.sees( xpos(), ypos(), tx, ty, range_min, bresenham_slope );
        }
    } else {
        return false;
    }
}

Creature *Creature::auto_find_hostile_target( int range, int &boo_hoo, int area )
{
    int t;
    Creature *target = nullptr;
    player &u = g->u; // Could easily protect something that isn't the player
    const int iff_dist = ( range + area ) * 3 / 2 + 6; // iff check triggers at this distance
    int iff_hangle = 15 + area; // iff safety margin (degrees). less accuracy, more paranoia
    float best_target_rating = -1.0f; // bigger is better
    int u_angle = 0;         // player angle relative to turret
    boo_hoo = 0;         // how many targets were passed due to IFF. Tragically.
    bool area_iff = false;   // Need to check distance from target to player
    bool angle_iff = true;   // Need to check if player is in a cone between us and target
    int pldist = rl_dist( pos(), g->u.pos() );
    int part;
    vehicle *in_veh = is_fake() ? g->m.veh_at( xpos(), ypos(), part ) : nullptr;
    if( pldist < iff_dist && g->sees_u(xpos(), ypos(), t) ) {
        area_iff = area > 0;
        angle_iff = true;
        // Player inside vehicle won't be hit by shots from the roof,
        // so we can fire "through" them just fine.
        if( in_veh && g->m.veh_at( u.posx, u.posy, part ) == in_veh && in_veh->is_inside( part ) ) {
            angle_iff = false; // No angle IFF, but possibly area IFF
        } else if( pldist < 3 ) {
            iff_hangle = (pldist == 2 ? 30 : 60);    // granularity increases with proximity
        }
        u_angle = g->m.coord_to_angle(xpos(), ypos(), u.posx, u.posy);
    }
    std::vector<Creature*> targets;
    for (size_t i = 0; i < g->num_zombies(); i++) {
        monster &m = g->zombie(i);
        if( m.friendly != 0 ) {
            // friendly to the player, not a target for us
            continue;
        }
        targets.push_back( &m );
    }
    for( auto &p : g->active_npc ) {
        if( p->attitude != NPCATT_KILL ) {
            // friendly to the player, not a target for us
            continue;
        }
        targets.push_back( p );
    }
    for( auto &m : targets ) {
        int t;
        if( !sees( *m, t ) ) {
            // can't see nor sense it
            continue;
        }
        int dist = rl_dist( pos(), m->pos() ) + 1; // rl_dist can be 0
        if( dist > range || dist < area ) {
            // Too near or too far
            continue;
        }
        // Prioritize big, armed and hostile stuff
        float mon_rating = m->power_rating();
        float target_rating = mon_rating / dist;
        if( mon_rating <= 0 ) {
            // We wouldn't attack it even if it was hostile
            continue;
        }

        if( in_veh != nullptr && g->m.veh_at( m->xpos(), m->ypos(), part ) == in_veh ) {
            // No shooting stuff on vehicle we're a part of
            continue;
        }
        if( area_iff > 0 && rl_dist( u.posx, u.posy, m->xpos(), m->ypos() ) <= area ) {
            // Player in AoE
            if( mon_rating > 1 ) {
                boo_hoo++;
            }
            continue;
        }
        if( angle_iff ) {
            int tangle = g->m.coord_to_angle(xpos(), ypos(), m->xpos(), m->ypos());
            int diff = abs(u_angle - tangle);
            // Player is in the angle and not too far behind the target
            if( ( diff + iff_hangle > 360 || diff < iff_hangle ) &&
                ( dist * 3 / 2 + 6 > pldist ) ) {
                // Don't inform of very weak targets we wouldn't shoot anyway
                if( mon_rating > 1 ) {
                    boo_hoo++;
                }
                continue;
            }
        }
        if( ( mon_rating + 2 ) / dist <= best_target_rating ) {
            // "Would we skip the target even if it was hostile?"
            // Helps avoid (possibly expensive) attitude calculation
            continue;
        }
        if( m->attitude_to( u ) == A_HOSTILE ) {
            target_rating = ( mon_rating + 2 ) / dist;
        }
        if( target_rating <= best_target_rating ) {
            continue; // Handle this late so that boo_hoo++ can happen
        }

        target = m;
        best_target_rating = target_rating;
    }
    return target;
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
    bool u_see_this = g->u_see(*this);
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
    // Apply damage multiplier from critical hits or grazes after all other modifications.
    const int adjusted_damage = du.amount * du.damage_multiplier;
    switch (du.type) {
    case DT_BASH:
        damage += adjusted_damage;
        // add up pain before using mod_pain since certain traits modify that
        pain += adjusted_damage / 4;
        mod_moves(-rng(0, damage * 2)); // bashing damage reduces moves
        break;
    case DT_CUT:
        damage += adjusted_damage;
        pain += (adjusted_damage + sqrt(double(adjusted_damage))) / 4;
        break;
    case DT_STAB: // stab differs from cut in that it ignores some armor
        damage += adjusted_damage;
        pain += (adjusted_damage + sqrt(double(adjusted_damage))) / 4;
        break;
    case DT_HEAT: // heat damage sets us on fire sometimes
        damage += adjusted_damage;
        pain += adjusted_damage / 4;
        if( rng(0, 100) < adjusted_damage ) {
            add_effect("onfire", rng(1, 3));
        }
        break;
    case DT_ELECTRIC: // electrical damage slows us a lot
        damage += adjusted_damage;
        pain += adjusted_damage / 4;
        mod_moves(-adjusted_damage * 100);
        break;
    case DT_COLD: // cold damage slows us a bit and hurts less
        damage += adjusted_damage;
        pain += adjusted_damage / 6;
        mod_moves(-adjusted_damage * 80);
        break;
    default:
        damage += adjusted_damage;
        pain += adjusted_damage / 4;
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
bool Creature::move_effects()
{
    return true;
}

void Creature::add_eff_effects(effect e, bool reduced)
{
    (void)e;
    (void)reduced;
    return;
}
 
void Creature::add_effect(efftype_id eff_id, int dur, body_part bp, bool permanent, int intensity)
{
    // Mutate to a main (HP'd) body_part if necessary.
    if (effect_types[eff_id].get_main_parts()) {
        bp = mutate_to_main_part(bp);
    }

    bool found = false;
    // Check if we already have it
    auto matching_map = effects.find(eff_id);
    if (matching_map != effects.end()) {
        auto found_effect = effects[eff_id].find(bp);
        if (found_effect != effects[eff_id].end()) {
            found = true;
            effect &e = found_effect->second;
            // If we do, mod the duration, factoring in the mod value
            e.mod_duration(dur * e.get_dur_add_perc() / 100);
            // Limit to max duration
            if (e.get_max_duration() > 0 && e.get_duration() > e.get_max_duration()) {
                e.set_duration(e.get_max_duration());
            }
            // Adding a permanent effect makes it permanent
            if( e.is_permanent() ) {
                e.pause_effect();
            }
            // Set intensity if value is given
            if (intensity > 0) {
                e.set_intensity(intensity);
            // Else intensity uses the type'd step size if it already exists
            } else if (e.get_int_add_val() != 0) {
                e.mod_intensity(e.get_int_add_val());
            }
            
            // Bound intensity by [1, max intensity]
            if (e.get_intensity() < 1) {
                add_msg( m_debug, "Bad intensity, ID: %s", e.get_id().c_str() );
                e.set_intensity(1);
            } else if (e.get_intensity() > e.get_max_intensity()) {
                e.set_intensity(e.get_max_intensity());
            }
        }
    }
    
    if (found == false) {
        // If we don't already have it then add a new one
        if (effect_types.find(eff_id) == effect_types.end()) {
            return;
        }
        effect new_eff(&effect_types[eff_id], dur, bp, permanent, intensity);
        effect &e = new_eff;
        // Bound to max duration
        if (e.get_max_duration() > 0 && e.get_duration() > e.get_max_duration()) {
            e.set_duration(e.get_max_duration());
        }
        // Bound new effect intensity by [1, max intensity]
        if (new_eff.get_intensity() < 1) {
            add_msg( m_debug, "Bad intensity, ID: %s", new_eff.get_id().c_str() );
            new_eff.set_intensity(1);
        } else if (new_eff.get_intensity() > new_eff.get_max_intensity()) {
            new_eff.set_intensity(new_eff.get_max_intensity());
        }
        effects[eff_id][bp] = new_eff;
        if (is_player()) {
            // Only print the message if we didn't already have it
            if(effect_types[eff_id].get_apply_message() != "") {
                     add_msg(effect_types[eff_id].gain_game_message_type(),
                             _(effect_types[eff_id].get_apply_message().c_str()));
            }
            add_memorial_log(pgettext("memorial_male",
                                           effect_types[eff_id].get_apply_memorial_log().c_str()),
                                  pgettext("memorial_female",
                                           effect_types[eff_id].get_apply_memorial_log().c_str()));
        }
        // Perform any effect addition effects.
        bool reduced = has_effect(e.get_resist_effect()) || has_trait(e.get_resist_trait());
        add_eff_effects(e, reduced);
    }
}
bool Creature::add_env_effect(efftype_id eff_id, body_part vector, int strength, int dur,
                              body_part bp, bool permanent, int intensity)
{
    if (dice(strength, 3) > dice(get_env_resist(vector), 3)) {
        // Only add the effect if we fail the resist roll
        add_effect(eff_id, dur, bp, permanent, intensity);
        return true;
    } else {
        return false;
    }
}
void Creature::clear_effects()
{
    effects.clear();
}
bool Creature::remove_effect(efftype_id eff_id, body_part bp)
{
    if (!has_effect(eff_id, bp)) {
        //Effect doesn't exist, so do nothing
        return false;
    }
    
    if (is_player()) {
        // Print the removal message and add the memorial log if needed
        if(effect_types[eff_id].get_remove_message() != "") {
            add_msg(effect_types[eff_id].lose_game_message_type(),
                         _(effect_types[eff_id].get_remove_message().c_str()));
        }
        add_memorial_log(pgettext("memorial_male",
                                       effect_types[eff_id].get_remove_memorial_log().c_str()),
                              pgettext("memorial_female",
                                       effect_types[eff_id].get_remove_memorial_log().c_str()));
    }
    
    // num_bp means remove all of a given effect id
    if (bp == num_bp) {
        effects.erase(eff_id);
    } else {
        effects[eff_id].erase(bp);
        // If there are no more effects of a given type remove the type map
        if (effects[eff_id].empty()) {
            effects.erase(eff_id);
        }
    }
    return true;
}
bool Creature::has_effect(efftype_id eff_id, body_part bp) const
{
    // num_bp means anything targeted or not
    if (bp == num_bp) {
        return effects.find( eff_id ) != effects.end();
    } else {
        auto got_outer = effects.find(eff_id);
        if(got_outer != effects.end()) {
            auto got_inner = got_outer->second.find(bp);
            if (got_inner != got_outer->second.end()) {
                return true;
            }
        }
        return false;
    }
}
effect Creature::get_effect(efftype_id eff_id, body_part bp) const
{
    auto got_outer = effects.find(eff_id);
    if(got_outer != effects.end()) {
        auto got_inner = got_outer->second.find(bp);
        if (got_inner != got_outer->second.end()) {
            return got_inner->second;
        }
    }
    return effect();
}
int Creature::get_effect_dur(efftype_id eff_id, body_part bp) const
{
    if(has_effect(eff_id, bp)) {
        effect tmp = get_effect(eff_id, bp);
        return tmp.get_duration();
    }
    return 0;
}
int Creature::get_effect_int(efftype_id eff_id, body_part bp) const
{
    if(has_effect(eff_id, bp)) {
        effect tmp = get_effect(eff_id, bp);
        return tmp.get_intensity();
    }
    return 0;
}
void Creature::process_effects()
{
    // id's and body_part's of all effects to be removed. If we ever get player or
    // monster specific removals these will need to be moved down to that level and then
    // passed in to this function.
    std::vector<std::string> rem_ids;
    std::vector<body_part> rem_bps;
    
    // Decay/removal of effects
    for( auto &elem : effects ) {
        for( auto &_it : elem.second ) {
            // Add any effects that others remove to the removal list
            if( _it.second.get_removes_effect() != "" ) {
                rem_ids.push_back( _it.second.get_removes_effect() );
                rem_bps.push_back(num_bp);
            }
            // Run decay effects
            _it.second.decay( rem_ids, rem_bps, calendar::turn, is_player() );
        }
    }
    
    // Actually remove effects. This should be the last thing done in process_effects().
    for (size_t i = 0; i < rem_ids.size(); ++i) {
        remove_effect( rem_ids[i], rem_bps[i] );
    }
}

bool Creature::has_trait(const std::string &flag) const
{
    (void)flag;
    return false;
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

    add_msg( m_debug, "Health: %d, Health mod: %d", get_healthy(), get_healthy_mod());
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
    // Pain should never go negative
    if (pain < 0) {
        pain = 0;
    }
}
void Creature::mod_moves(int nmoves)
{
    moves += nmoves;
}
void Creature::set_moves(int nmoves)
{
    moves = nmoves;
}

bool Creature::in_sleep_state() const
{
    return has_effect("sleep") || has_effect("lying_down");
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

    add_msg( m_debug, "source size = %d", source->get_size() );
    add_msg( m_debug, "target size = %d", get_size() );
    add_msg( m_debug, "difference = %d", szdif );

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
    add_msg( m_debug, "eyes = %f", hit_weights.at( bp_eyes ) );
    add_msg( m_debug, "head = %f", hit_weights.at( bp_head ) );
    add_msg( m_debug, "torso = %f", hit_weights.at( bp_torso ) );
    add_msg( m_debug, "arm_l = %f", hit_weights.at( bp_arm_l ) );
    add_msg( m_debug, "arm_r = %f", hit_weights.at( bp_arm_r ) );
    add_msg( m_debug, "leg_l = %f", hit_weights.at( bp_leg_l ) );
    add_msg( m_debug, "leg_r = %f", hit_weights.at( bp_leg_r ) );

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

bool Creature::compare_by_dist_to_point::operator()( const Creature* const a, const Creature* const b ) const
{
    return rl_dist( a->pos(), center ) < rl_dist( b->pos(), center );
}
