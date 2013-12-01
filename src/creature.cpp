#include "creature.h"
#include "output.h"
#include "game.h"
#include <algorithm>

Creature::Creature() {};

Creature::Creature(const Creature & rhs) {};

void Creature::normalize(game* g) {
}

void Creature::reset(game *g) {
    // Reset our stats to normal levels
    // Any persistent buffs/debuffs will take place in disease.h,
    // player::suffer(), etc.

    // First reset all bonuses to 0 and mults to 1.0
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

    // then repopulate the bonus fields
    for (std::vector<effect>::iterator it = effects.begin();
            it != effects.end(); ++it) {
        it->do_effect(g, *this);
    }
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
// utility function for process_effects
bool is_expired_effect(effect& e) { return e.get_duration() <= 0; }

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
/*
 * Innate stats getters
 */

// get_stat() always gets total (current) value, NEVER just the base
// get_stat_bonus() is always just the bonus amount
int Creature::get_str() {
    return str_max + str_bonus;
}
int Creature::get_dex() {
    return dex_max + dex_bonus;
}
int Creature::get_per() {
    return per_max + per_bonus;
}
int Creature::get_int() {
    return int_max + int_bonus;
}

int Creature::get_str_base() {
    return str_max;
}
int Creature::get_dex_base() {
    return dex_max;
}
int Creature::get_per_base() {
    return per_max;
}
int Creature::get_int_base() {
    return int_max;
}



int Creature::get_str_bonus() {
    return str_bonus;
}
int Creature::get_dex_bonus() {
    return dex_bonus;
}
int Creature::get_per_bonus() {
    return per_bonus;
}
int Creature::get_int_bonus() {
    return int_bonus;
}

int Creature::get_num_blocks() {
    return num_blocks + num_blocks_bonus;
}
int Creature::get_num_dodges() {
    return num_dodges + num_dodges_bonus;
}
int Creature::get_num_blocks_bonus() {
    return num_blocks_bonus;
}
int Creature::get_num_dodges_bonus() {
    return num_dodges_bonus;
}

// TODO: implement and actually use these for bash and cut armor,
// probably needs prototype change to support bodypart
//
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
    return speed_base + get_speed_bonus();
}
int Creature::get_dodge() {
    int ret = (get_dex() / 2) + int(get_speed() / 150) //Faster = small dodge advantage
        + get_dodge_bonus();
    return ret;
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



