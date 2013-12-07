#include "item.h"
#include "creature.h"
#include "output.h"
#include "game.h"
#include <algorithm>
#include <numeric>

Creature::Creature() {};

Creature::Creature(const Creature & rhs) {};

void Creature::normalize(game* g) {
}

void Creature::reset(game *g) {
    reset_bonuses(g);
    reset_stats(g);
}
void Creature::reset_bonuses(game *g) {
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

    speed_base = 100;
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
void Creature::reset_stats(game *g) {
    // Reset our stats to normal levels
    // Any persistent buffs/debuffs will take place in disease.h,
    // player::suffer(), etc.

    // repopulate the stat fields
    process_effects(g);
    str_cur = str_max + get_str_bonus();
    dex_cur = dex_max + get_dex_bonus();
    per_cur = per_max + get_per_bonus();
    int_cur = int_max + get_int_bonus();

    // Floor for our stats.  No stat changes should occur after this!
    if (dex_cur < 0)
        dex_cur = 0;
    if (str_cur < 0)
        str_cur = 0;
    if (per_cur < 0)
        per_cur = 0;
    if (int_cur < 0)
        int_cur = 0;

    // add an appropriate number of moves
    moves = get_speed();
}

/*
 * Damage-related functions
 */

int Creature::projectile_attack(game *g, projectile &proj, int targetx, int targety,
        double missed_by) {
    std::vector<point> trajectory = line_to(xpos(),ypos(),targetx,targety,0);

    // Set up a timespec for use in the nanosleep function below
    timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = BULLET_SPEED;

    int dam = proj.impact.total_damage() + proj.payload.total_damage();
    it_ammo *curammo = proj.ammo;
    item ammotmp = item(curammo, 0);
    ammotmp.charges = 1;

    //bool is_bolt = (curammo->type == "bolt" || curammo->type == "arrow");

    bool missed = missed_by >= 0.8;

    // Trace the trajectory, doing damage in order
    int tx = trajectory[0].x;
    int ty = trajectory[0].y;
    int px = trajectory[0].x;
    int py = trajectory[0].y;

    for (int i = 0; i < trajectory.size() && (dam > 0 || (proj.proj_effects.count("FLAME"))); i++) {
        px = tx;
        py = ty;
        (void) px;
        (void) py;
        tx = trajectory[i].x;
        ty = trajectory[i].y;
        // Drawing the bullet uses player u, and not player p, because it's drawn
        // relative to YOUR position, which may not be the gunman's position.
        g->draw_bullet(*this, tx, ty, i, trajectory, proj.proj_effects.count("FLAME")? '#':'*', ts);

        if (dam <= 0 && !(proj.proj_effects.count("FLAME"))) { // Ran out of momentum.
            return 0;
        }

        // If there's a monster in the path of our bullet, and either our aim was true,
        //  OR it's not the monster we were aiming at and we were lucky enough to hit it
        int mondex = g->mon_at(tx, ty);
        // If we shot us a monster...
        /* TODO: implement underground etc flags in Creature
        if (mondex != -1 && ((!zombie(mondex).digging()) ||
                            rl_dist(xpos(), ypos(), g->zombie(mondex).xpos(),
                                    g->zombie(mondex).ypos()) <= 1) &&
                                    */
        if (mondex != -1 &&
            ((!missed && i == trajectory.size() - 1) ||
            one_in((5 - int(g->zombie(mondex).type->size)))) ) {
            monster &z = g->zombie(mondex);

            double goodhit = missed_by;
            if (i < trajectory.size() - 1) { // Unintentional hit
                goodhit = double(rand() / (RAND_MAX + 1.0)) / 2;
            }

            // Penalize for the monster's speed
            if (z.speed > 80) {
                goodhit *= double( double(z.speed) / 80.0);
            }

            /*
            shoot_monster(this, p, z, dam, goodhit, weapon, proj_effects);
            */
            g->add_msg(_("You hit the %s for %d damage."),
                    z.disp_name().c_str(), dam);
            damage_instance d = damage_instance::physical(0,dam,0);
            z.deal_damage(g, this, bp_torso, 3, d);
            std::vector<point> blood_traj = trajectory;
            blood_traj.insert(blood_traj.begin(), point(xpos(), ypos()));
            //splatter(this, blood_traj, dam, &z); TODO: add splatter effects
            //back in
            dam = 0;
        } else {
            g->m.shoot(g, tx, ty, dam, i == trajectory.size() - 1, proj.proj_effects);
        }
    } // Done with the trajectory!

    ammo_effects(g, tx, ty, proj.proj_effects);
    /* TODO: add bounce effects back in
    if (proj_effects.count("BOUNCE")) {
        for (unsigned long int i = 0; i < num_zombies(); i++) {
            monster &z = zombie(i);
            // search for monsters in radius 4 around impact site
            if (rl_dist(z.posx(), z.posy(), tx, ty) <= 4) {
                // don't hit targets that have already been hit
                if (!z.has_effect("effect_bounced") && !z.dead) {
                    add_msg(_("The attack bounced to %s!"), z.name().c_str());
                    trajectory = line_to(tx, ty, z.posx(), z.posy(), 0);
                    if (weapon->charges > 0) {
                        fire(p, z.posx(), z.posy(), trajectory, false);
                    }
                    break;
                }
            }
        }
    }
    */

    return 0;
}


// TODO: this is a shim for the currently existing calls to Creature::hit,
// start phasing them out
int Creature::hit(game *g, Creature* source, body_part bphurt, int side,
        int dam, int cut) {
    damage_instance d;
    d.add_damage(DT_BASH, dam);
    d.add_damage(DT_CUT, cut);
    dealt_damage_instance dealt_dams = deal_damage(g, source, bphurt, side, d);

    return dealt_dams.total_damage();
}

int Creature::deal_melee_attack(game* g, Creature* source, int hitroll, bool critical_hit,
        damage_instance& d, dealt_damage_instance &dealt_dam) {
    int dodgeroll = dodge_roll();
    int hit_spread = hitroll - dodgeroll;
    bool missed = hit_spread <= 0;

    if (missed) return hit_spread;

    //bool critical_hit = hit_spread > 30; //scored_crit(dodgeroll);

    body_part bp_hit;
    int side = rng(0, 1);
    int hit_value = hitroll + rng(-10, 10);
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

    // Bashing crit
    if (critical_hit) {
        int turns_stunned = (d.type_damage(DT_BASH) + hit_spread)/20;
        if (turns_stunned > 6)
            turns_stunned = 6;
        if (turns_stunned > 0) {
            add_effect("effect_stunned", turns_stunned);
        }
    }

    // Stabbing effects
    int stab_moves = rng(d.type_damage(DT_STAB) / 2, d.type_damage(DT_STAB) * 1.5);
    if (critical_hit) {
        stab_moves *= 1.5;
    }
    if (stab_moves >= 150) {
        if (is_player()) {
            // can the player force their self to the ground? probably not.
            g->add_msg_if_npc(source, _("<npcname> forces you to the ground!"));
        } else {
            g->add_msg_player_or_npc(source, _("You force %s to the ground!"),
                                     _("<npcname> forces %s to the ground!"),
                                     disp_name().c_str() );
        }
        add_effect("effect_downed", 1);
        mod_moves(-stab_moves / 2);
    } else
        mod_moves(-stab_moves);
    block_hit(g, bp_hit, side, d);

    dealt_dam = deal_damage(g, this, bp_hit, side, d);

    /* TODO: add grabs n shit back in
    if (allow_special && technique.grabs) {
        // TODO: make this depend on skill (through grab_resist stat) again
        if (t.get_grab_resist() > 0 &&
                dice(t.get_dex() , 12) >
                dice(get_dex(), 10)) {
            g->add_msg_player_or_npc(&t, _("You break the grab!"),
                                        _("<npcname> breaks the grab!"));
        } else if (!unarmed_attack()) {
            // Move our weapon to a temp slot, if it's not unarmed
            item tmpweap = remove_weapon();
            melee_attack(g, t, false); // False means a second grab isn't allowed
            weapon = tmpweap;
        } else
            melee_attack(g, t, false); // False means a second grab isn't allowed
    }
    */

    return hit_spread;
}

int Creature::deal_projectile_attack(game* g, Creature* source, float missed_by, bool dodgeable,
        damage_instance& d, dealt_damage_instance &dealt_dam) {

    return 0;
}

dealt_damage_instance Creature::deal_damage(game* g, Creature* source, body_part bp, int side,
        damage_instance& d) {
    int total_damage = 0;
    int total_pain = 0;

    std::vector<int> dealt_dams(NUM_DT, 0);

    absorb_hit(g, bp, side, d);

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
    apply_damage(g, source, bp, side, total_damage);
    return dealt_damage_instance(dealt_dams);
}
void Creature::deal_damage_handle_type(const damage_unit& du, body_part bp, int& damage, int& pain) {
    switch (du.type) {
    case DT_BASH:
        damage += du.amount;
        pain += du.amount / 4; // add up pain before using mod_pain since certain traits modify that
        mod_moves(-rng(0,damage*2)); // bashing damage reduces moves
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
        if (rng(0,100) > (100 - 400/(du.amount+3)))
            add_effect("effect_onfire", rng(1,3));
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

bool Creature::is_warm() {
    return true;
}

/*
 * Effect-related functions
 */
// Some utility functions for effects
class is_id_functor { // functor for remove/has_effect, give c++11 lambdas pls
    std::string id;
    public:
        is_id_functor(efftype_id rem_id) : id(rem_id) {}
        bool operator() (effect& e) { return e.get_id() == id; }
};
bool is_expired_effect(effect& e) { // utility function for process_effects
    if (e.get_duration() <= 0) {
        g->add_msg_string(e.get_effect_type()->get_remove_message());
        g->u.add_memorial_log(e.get_effect_type()->get_remove_memorial_log().c_str());
        return true;
    } else
        return false;
}

void Creature::add_effect(efftype_id eff_id, int dur) {
    // check if we already have it
    std::vector<effect>::iterator first_eff =
        std::find_if(effects.begin(), effects.end(), is_id_functor(eff_id));

    if (first_eff != effects.end()) {
        // if we do, mod the duration
        first_eff->mod_duration(dur);
    } else {
        // if we don't already have it then add a new one
        if (effect_types.find(eff_id) == effect_types.end())
            return;
        effect new_eff(&effect_types[eff_id], dur);
        effects.push_back(new_eff);
    }

    if (is_player()) {
        g->add_msg_string(effect_types[eff_id].get_apply_message());
        g->u.add_memorial_log(effect_types[eff_id].get_apply_memorial_log().c_str());
    }
}
bool Creature::add_env_effect(efftype_id eff_id, body_part vector, int strength, int dur) {
    if (dice(1, 3) > dice(get_env_resist(vector), 3)) {
        add_effect(eff_id, dur);
        return true;
    } else
        return false;
}
void Creature::clear_effects() {
    effects.clear();
}
void Creature::remove_effect(efftype_id rem_id) {
    // remove all effects with this id
    effects.erase(std::remove_if(effects.begin(), effects.end(),
                       is_id_functor(rem_id)), effects.end());
}
bool Creature::has_effect(efftype_id eff_id) {
    // return if any effect in effects has this id
    return (std::find_if(effects.begin(), effects.end(), is_id_functor(eff_id))
        != effects.end());
}
void Creature::process_effects(game* g) {
    for (std::vector<effect>::iterator it = effects.begin();
            it != effects.end(); ++it) {
        if (it->get_duration() > 0) {
            it->mod_duration(-1);
            if (g->debugmon)
                debugmsg("Duration %d", it->get_duration());
        }
    }
    effects.erase(std::remove_if(effects.begin(), effects.end(),
                       is_expired_effect), effects.end());
    /* TODO: use this instead when we switch to c++11 ;)
    effects.erase(std::remove_if(effects.begin(), effects.end(),
                       [](effect& e) { return e.get_duration() <= 0; }), effects.end());
                       */
}


void Creature::mod_pain(int npain) {
    pain += npain;
}
void Creature::mod_moves(int nmoves) {
    moves += nmoves;
}

/*
 * Innate stats getters
 */

// get_stat() always gets total (current) value, NEVER just the base
// get_stat_bonus() is always just the bonus amount
int Creature::get_str() const {
    return str_max + str_bonus;
}
int Creature::get_dex() const {
    return dex_max + dex_bonus;
}
int Creature::get_per() const {
    return per_max + per_bonus;
}
int Creature::get_int() const {
    return int_max + int_bonus;
}

int Creature::get_str_base() const {
    return str_max;
}
int Creature::get_dex_base() const {
    return dex_max;
}
int Creature::get_per_base() const {
    return per_max;
}
int Creature::get_int_base() const {
    return int_max;
}



int Creature::get_str_bonus() const {
    return str_bonus;
}
int Creature::get_dex_bonus() const {
    return dex_bonus;
}
int Creature::get_per_bonus() const {
    return per_bonus;
}
int Creature::get_int_bonus() const {
    return int_bonus;
}

int Creature::get_num_blocks() const {
    return num_blocks + num_blocks_bonus;
}
int Creature::get_num_dodges() const {
    return num_dodges + num_dodges_bonus;
}
int Creature::get_num_blocks_bonus() const {
    return num_blocks_bonus;
}
int Creature::get_num_dodges_bonus() const {
    return num_dodges_bonus;
}

// currently this is expected to be overridden to actually have use
int Creature::get_env_resist(body_part bp) {
    return 0;
}
int Creature::get_armor_bash(body_part bp) {
    return armor_bash_bonus;
}
int Creature::get_armor_cut(body_part bp) {
    return armor_cut_bonus;
}
int Creature::get_armor_bash_base(body_part bp) {
    return armor_bash_bonus;
}
int Creature::get_armor_cut_base(body_part bp) {
    return armor_cut_bonus;
}
int Creature::get_armor_bash_bonus() {
    return armor_bash_bonus;
}
int Creature::get_armor_cut_bonus() {
    return armor_cut_bonus;
}

int Creature::get_speed() {
    return get_speed_base() + get_speed_bonus();
}
int Creature::get_dodge() {
    return get_dodge_base() + get_dodge_bonus();
}
int Creature::get_hit() {
    return get_hit_base() + get_hit_bonus();
}

int Creature::get_speed_base() {
    return speed_base;
}
int Creature::get_dodge_base() {
    return (get_dex() / 2) + int(get_speed() / 150); //Faster = small dodge advantage
}
int Creature::get_hit_base() {
    return (get_dex() / 2) + 1;
}
int Creature::get_speed_bonus() {
    return speed_bonus;
}
int Creature::get_dodge_bonus() {
    return dodge_bonus;
}
int Creature::get_block_bonus() {
    return block_bonus; //base is 0
}
int Creature::get_hit_bonus() {
    return hit_bonus; //base is 0
}
int Creature::get_bash_bonus() {
    return bash_bonus;
}
int Creature::get_cut_bonus() {
    return cut_bonus;
}

float Creature::get_bash_mult() {
    return bash_mult;
}
float Creature::get_cut_mult() {
    return cut_mult;
}

bool Creature::get_melee_quiet() {
    return melee_quiet;
}
int Creature::get_grab_resist() {
    return grab_resist;
}
int Creature::get_throw_resist() {
    return throw_resist;
}

/*
 * Innate stats setters
 */

void Creature::set_str_bonus(int nstr) {
    str_bonus = nstr;
}
void Creature::set_dex_bonus(int ndex) {
    dex_bonus = ndex;
}
void Creature::set_per_bonus(int nper) {
    per_bonus = nper;
}
void Creature::set_int_bonus(int nint) {
    int_bonus = nint;
}
void Creature::mod_str_bonus(int nstr) {
    str_bonus += nstr;
}
void Creature::mod_dex_bonus(int ndex) {
    dex_bonus += ndex;
}
void Creature::mod_per_bonus(int nper) {
    per_bonus += nper;
}
void Creature::mod_int_bonus(int nint) {
    int_bonus += nint;
}

void Creature::set_num_blocks_bonus(int nblocks) {
    num_blocks_bonus = nblocks;
}
void Creature::set_num_dodges_bonus(int ndodges) {
    num_dodges_bonus = ndodges;
}

void Creature::set_armor_bash_bonus(int nbasharm) {
    armor_bash_bonus = nbasharm;
}
void Creature::set_armor_cut_bonus(int ncutarm) {
    armor_cut_bonus = ncutarm;
}

void Creature::set_speed_bonus(int nspeed) {
    speed_bonus = nspeed;
}
void Creature::set_dodge_bonus(int ndodge) {
    dodge_bonus = ndodge;
}
void Creature::set_block_bonus(int nblock) {
    block_bonus = nblock;
}
void Creature::set_hit_bonus(int nhit) {
    hit_bonus = nhit;
}
void Creature::set_bash_bonus(int nbash) {
    bash_bonus = nbash;
}
void Creature::set_cut_bonus(int ncut) {
    cut_bonus = ncut;
}
void Creature::mod_speed_bonus(int nspeed) {
    speed_bonus += nspeed;
}
void Creature::mod_dodge_bonus(int ndodge) {
    dodge_bonus += ndodge;
}
void Creature::mod_block_bonus(int nblock) {
    block_bonus += nblock;
}
void Creature::mod_hit_bonus(int nhit) {
    hit_bonus += nhit;
}
void Creature::mod_bash_bonus(int nbash) {
    bash_bonus += nbash;
}
void Creature::mod_cut_bonus(int ncut) {
    cut_bonus += ncut;
}

void Creature::set_bash_mult(float nbashmult) {
    bash_mult = nbashmult;
}
void Creature::set_cut_mult(float ncutmult) {
    cut_mult = ncutmult;
}

void Creature::set_melee_quiet(bool nquiet) {
    melee_quiet = nquiet;
}
void Creature::set_grab_resist(int ngrabres) {
    grab_resist = ngrabres;
}
void Creature::set_throw_resist(int nthrowres) {
    throw_resist = nthrowres;
}

/*
 * Drawing-related functions
 */
void Creature::draw(WINDOW *w, int player_x, int player_y, bool inverted)
{
    int draw_x = getmaxx(w)/2 + xpos() - player_x;
    int draw_y = getmaxy(w)/2 + ypos() - player_y;
    if(inverted){
        mvwputch_inv(w, draw_y, draw_x, basic_symbol_color(), symbol());
    } else if(is_symbol_highlighted()){
        mvwputch_hi(w, draw_y, draw_x, basic_symbol_color(), symbol());
    } else {
        mvwputch(w, draw_y, draw_x, symbol_color(), symbol() );
    }
}

nc_color Creature::basic_symbol_color(){
    return c_ltred;
}

nc_color Creature::symbol_color()
{
    return symbol_color();
}

bool Creature::is_symbol_highlighted()
{
    return false;
}

char Creature::symbol()
{
    return '?';
}
