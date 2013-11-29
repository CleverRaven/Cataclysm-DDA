#include "creature.h"

creature::creature() {};

creature::creature(const creature & rhs) {};

void creature::normalize(game* g) {
}

void creature::reset(game *g) {
    // Reset our stats to normal levels
    // Any persistent buffs/debuffs will take place in disease.h,
    // player::suffer(), etc.

    for (std::vector<effect>::iterator it = effects.begin();
            it != effects.end(); ++it) {
        it->do_effects(g, *this);
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

void add_effect(std::string eff_id, int dur) {
    return;
}
bool has_effect(effect& eff) {
    return true;
}

/*
 * Innate stats getters
 */

// get_stat() always gets total (current) value, NEVER just the base
// get_stat_bonus() is always just the bonus amount
int creature::get_str() {
    return str_max + str_bonus;
}
int creature::get_dex() {
    return dex_max + dex_bonus;
}
int creature::get_per() {
    return per_max + per_bonus;
}
int creature::get_int() {
    return int_max + int_bonus;
}

int creature::get_str_bonus() {
    return str_bonus;
}
int creature::get_dex_bonus() {
    return dex_bonus;
}
int creature::get_per_bonus() {
    return per_bonus;
}
int creature::get_int_bonus() {
    return int_bonus;
}

int creature::get_num_blocks() {
    return num_blocks + num_blocks_bonus;
}
int creature::get_num_dodges() {
    return num_dodges + num_dodges_bonus;
}
int creature::get_num_blocks_bonus() {
    return num_blocks_bonus;
}
int creature::get_num_dodges_bonus() {
    return num_dodges_bonus;
}

// TODO: implement and actually use these for bash and cut armor,
// probably needs prototype change to support bodypart
//
int creature::get_arm_bash_bonus() {
    return arm_bash_bonus;
}
int creature::get_arm_cut_bonus() {
    return arm_cut_bonus;
}

int creature::get_speed() {
    return speed_base + get_speed_bonus();
}
int creature::get_dodge() {
    int ret = (get_dex() / 2) + int(get_speed() / 150) //Faster = small dodge advantage
        + get_dodge_bonus();
    return ret;
}

int creature::get_speed_bonus() {
    return speed_bonus;
}
int creature::get_dodge_bonus() {
    return dodge_bonus;
}
int creature::get_block_bonus() {
    return block_bonus; //base is 0
}
int creature::get_hit_bonus() {
    return hit_bonus; //base is 0
}
int creature::get_bash_bonus() {
    return bash_bonus;
}
int creature::get_cut_bonus() {
    return cut_bonus;
}

float creature::get_bash_mult() {
    return bash_mult;
}
float creature::get_cut_mult() {
    return cut_mult;
}

bool creature::get_melee_quiet() {
    return melee_quiet;
}
int creature::get_throw_resist() {
    return throw_resist;
}

/*
 * Innate stats setters
 */

void creature::set_str_bonus(int nstr) {
    str_bonus = nstr;
}
void creature::set_dex_bonus(int ndex) {
    dex_bonus = ndex;
}
void creature::set_per_bonus(int nper) {
    per_bonus = nper;
}
void creature::set_int_bonus(int nint) {
    int_bonus = nint;
}
void creature::mod_str_bonus(int nstr) {
    str_bonus += nstr;
}
void creature::mod_dex_bonus(int ndex) {
    dex_bonus += ndex;
}
void creature::mod_per_bonus(int nper) {
    per_bonus += nper;
}
void creature::mod_int_bonus(int nint) {
    int_bonus += nint;
}

void creature::set_num_blocks_bonus(int nblocks) {
    num_blocks_bonus = nblocks;
}
void creature::set_num_dodges_bonus(int ndodges) {
    num_dodges_bonus = ndodges;
}

void creature::set_arm_bash_bonus(int nbasharm) {
    arm_bash_bonus = nbasharm;
}
void creature::set_arm_cut_bonus(int ncutarm) {
    arm_cut_bonus = ncutarm;
}

void creature::set_speed_bonus(int nspeed) {
    speed_bonus = nspeed;
}
void creature::set_dodge_bonus(int ndodge) {
    dodge_bonus = ndodge;
}
void creature::set_block_bonus(int nblock) {
    block_bonus = nblock;
}
void creature::set_hit_bonus(int nhit) {
    hit_bonus = nhit;
}
void creature::set_bash_bonus(int nbash) {
    bash_bonus = nbash;
}
void creature::set_cut_bonus(int ncut) {
    cut_bonus = ncut;
}

void creature::set_bash_mult(float nbashmult) {
    bash_mult = nbashmult;
}
void creature::set_cut_mult(float ncutmult) {
    cut_mult = ncutmult;
}

void creature::set_melee_quiet(bool nquiet) {
    melee_quiet = nquiet;
}
void creature::set_throw_resist(int nthrowres) {
    throw_resist = nthrowres;
}



