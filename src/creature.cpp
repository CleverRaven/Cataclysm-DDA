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
// TODO: this is a shim for the currently existing calls to Creature::hit,
// start phasing them out
int Creature::hit(game *g, Creature* source, body_part bphurt, int side,
        int dam, int cut) {
    damage_instance d;
    d.add_damage(DT_BASH, dam);
    d.add_damage(DT_CUT, cut);
    std::vector<int> dealt_dams = deal_damage(g, source, bphurt, side, d);

    return std::accumulate(dealt_dams.begin(),dealt_dams.end(),0);
}

std::vector<int> Creature::deal_damage(game* g, Creature* source, body_part bp, int side,
        const damage_instance& d) {
    int total_damage = 0;
    int total_pain = 0;

    std::vector<int> dealt_dams(NUM_DT, 0);

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
    return dealt_dams;
}
void Creature::deal_damage_handle_type(const damage_unit& du, body_part bp, int& damage, int& pain) {
    switch (du.type) {
    case DT_BASH:
        damage += du.amount -
            std::max(get_armor_bash(bp) - du.res_pen,0)*du.res_mult;
        pain += du.amount / 4;
        break;
    case DT_CUT:
        damage += du.amount -
            std::max(get_armor_cut(bp) - du.res_pen,0)*du.res_mult;
        pain += (du.amount + sqrt(double(du.amount))) / 4;
        break;
    case DT_STAB: // stab differs from cut in that it ignores some armor
        damage += du.amount -
            0.8*std::max(get_armor_cut(bp) - du.res_pen,0)*du.res_mult;
        pain += (du.amount + sqrt(double(du.amount))) / 4;
        break;
    default:
        damage += du.amount;
        pain += du.amount / 4;
    }
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
    int ret = (get_dex() / 2) + int(get_speed() / 150) //Faster = small dodge advantage
        + get_dodge_bonus();
    return ret;
}

int Creature::get_speed_base() {
    return speed_base;
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

//TODO: Finish this implementation
bool Creature::has_flag(m_flag flag_name)
{
    return false;
}
