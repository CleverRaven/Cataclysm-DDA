#ifndef _CREATURE_H_
#define _CREATURE_H_

#include "enums.h"
#include "pldata.h"
#include "skill.h"
#include "bionics.h"
#include "json.h"
#include "effect.h"
#include "bodypart.h"
#include "color.h"
#include "mtype.h"
#include <stdlib.h>
#include <string>
#include <vector>
#include <algorithm>

struct damage_unit {
    damage_type type;
    int amount;
    int res_pen;
    float res_mult;
    damage_unit(damage_type dt, int a, int rp, float rm) :
        type(dt),
        amount(a),
        res_pen(rp),
        res_mult(rm)
    { }
};


// a single atomic unit of damage from an attack. Can include multiple types
// of damage at different armor mitigation/penetration values
struct damage_instance {
    std::vector<damage_unit> damage_units;
    damage_instance() { }
    void add_damage(damage_type dt, int a, int rp = 0, float rm = 1.0f) {
        damage_unit du(dt,a,rp,rm);
        damage_units.push_back(du);
    }
};

class game;
class effect;

class Creature
{
    public:
        // TODO: fill these in
        Creature();
        Creature(const Creature & rhs);

        virtual std::string disp_name() = 0; // displayname for Creature
        virtual std::string skin_name() = 0; // name of outer layer, e.g. "armor plates"

        virtual bool is_player() { return false; }

        virtual void normalize(game* g); // recreate the Creature from scratch
        virtual void reset(game* g); // handle both reset steps. Call this function instead of reset_stats/bonuses
        virtual void reset_bonuses(game* g); // reset the value of all bonus fields to 0
        virtual void reset_stats(game* g); // prepare the Creature for the next turn
        virtual void die(game* g, Creature* killer) = 0;

        virtual int dodge_roll(game* g) = 0;

        virtual int hit_creature(game *g, Creature &t, bool allow_grab) = 0; // Returns a damage

        virtual int hit(game *g, Creature* source, body_part bphurt, int side,
                int dam, int cut);

        /*
        virtual int hit(game *g, body_part bphurt, int side,
                int dam, int cut) {
            hit(g,NULL,bphurt,side,dam,cut);
        }
        */

        virtual bool block_hit(game *g, body_part &bp_hit, int &side,
            int &bash_dam, int &cut_dam, int &stab_dam) = 0;

        // these also differ between player and monster (player takes
        // body_part) but we will use this for now
        // TODO: this is just a shim so knockbacks work
        virtual void knock_back_from(game *g, int posx, int posy) = 0;

        // TODO: remove this function in favor of deal/apply_damage
        virtual void hurt(game* g, body_part bp, int side, int dam) = 0;

        // deals the damage via an attack. Most sources of external damage
        // should use deal_damage
        virtual std::vector<int> deal_damage(game* g, Creature* source, body_part bp, int side, const damage_instance& d);
        // for each damage type, how much gets through and how much pain do we
        // accrue? mutates damage and pain
        virtual void deal_damage_handle_type(const damage_unit& du, body_part bp, int& damage, int& pain);
        // directly decrements the damage. ONLY handles damage, doesn't
        // increase pain, apply effects, etc
        virtual void apply_damage(game* g, Creature* source, body_part bp, int side, int amount) = 0;

        virtual bool is_on_ground() = 0;
        virtual bool has_weapon() = 0;
        // returns true iff health is zero or otherwise should be dead
        virtual bool is_dead_state() = 0;

        // xpos and ypos, because posx/posy are used as public variables in
        // player.cpp and therefore referenced everywhere
        virtual int xpos() = 0;
        virtual int ypos() = 0;

        // should replace both player.add_disease and monster.add_effect
        // these are nonvirtual since otherwise they can't be accessed with
        // the old add_effect
        void add_effect(efftype_id eff_id, int dur);
        bool add_env_effect(efftype_id eff_id, body_part vector, int strength, int dur); // gives chance to save via env resist, returns if successful
        void remove_effect(efftype_id eff_id);
        void clear_effects(); // remove all effects
        bool has_effect(efftype_id eff_id);

        virtual void process_effects(game* g); // runs all the effects on the Creature

        // not-quite-stats, maybe group these with stats later
        virtual void mod_pain(int npain);
        virtual void mod_moves(int nmoves);

        /*
         * getters for stats - combat-related stats will all be held within
         * the Creature and re-calculated during every normalize() call
         */
        virtual int get_str();
        virtual int get_dex();
        virtual int get_per();
        virtual int get_int();

        virtual int get_str_base();
        virtual int get_dex_base();
        virtual int get_per_base();
        virtual int get_int_base();

        virtual int get_str_bonus();
        virtual int get_dex_bonus();
        virtual int get_per_bonus();
        virtual int get_int_bonus();

        virtual int get_num_blocks();
        virtual int get_num_dodges();
        virtual int get_num_blocks_bonus();
        virtual int get_num_dodges_bonus();

        virtual int get_env_resist(body_part bp);

        virtual int get_armor_bash(body_part bp);
        virtual int get_armor_cut(body_part bp);
        virtual int get_armor_bash_base(body_part bp);
        virtual int get_armor_cut_base(body_part bp);
        virtual int get_armor_bash_bonus();
        virtual int get_armor_cut_bonus();

        virtual int get_speed();
        virtual int get_dodge();

        virtual int get_speed_base();
        virtual int get_speed_bonus();
        virtual int get_dodge_bonus();
        virtual int get_block_bonus();
        virtual int get_hit_bonus();
        virtual int get_bash_bonus();
        virtual int get_cut_bonus();

        virtual float get_bash_mult();
        virtual float get_cut_mult();

        virtual bool get_melee_quiet();
        virtual int get_grab_resist();
        virtual int get_throw_resist();

        /*
         * setters for stats and boni
         */
        virtual void set_str_bonus(int nstr);
        virtual void set_dex_bonus(int ndex);
        virtual void set_per_bonus(int nper);
        virtual void set_int_bonus(int nint);
        virtual void mod_str_bonus(int nstr);
        virtual void mod_dex_bonus(int ndex);
        virtual void mod_per_bonus(int nper);
        virtual void mod_int_bonus(int nint);

        virtual void set_num_blocks_bonus(int nblocks);
        virtual void set_num_dodges_bonus(int ndodges);

        virtual void set_armor_bash_bonus(int nbasharm);
        virtual void set_armor_cut_bonus(int ncutarm);

        virtual void set_speed_bonus(int nspeed);
        virtual void set_dodge_bonus(int ndodge);
        virtual void set_block_bonus(int nblock);
        virtual void set_hit_bonus(int nhit);
        virtual void set_bash_bonus(int nbash);
        virtual void set_cut_bonus(int ncut);
        virtual void mod_speed_bonus(int nspeed);
        virtual void mod_dodge_bonus(int ndodge);
        virtual void mod_block_bonus(int nblock);
        virtual void mod_hit_bonus(int nhit);
        virtual void mod_bash_bonus(int nbash);
        virtual void mod_cut_bonus(int ncut);

        virtual void set_bash_mult(float nbashmult);
        virtual void set_cut_mult(float ncutmult);

        virtual void set_melee_quiet(bool nquiet);
        virtual void set_grab_resist(int ngrabres);
        virtual void set_throw_resist(int nthrowres);

        // innate stats, slowly move these to protected as we rewrite more of
        // the codebase
        int str_max, dex_max, per_max, int_max,
            str_cur, dex_cur, per_cur, int_cur;

        int moves, pain;

        void draw(WINDOW* w, int plx, int ply, bool inv);

        //TODO: Actually implement this as a creature thing.
        virtual bool has_flag(m_flag flag_name);

    protected:
        std::vector<effect> effects;

        // used for innate bonuses like effects. weapon bonuses will be
        // handled separately

        int str_bonus;
        int dex_bonus;
        int per_bonus;
        int int_bonus;

        int num_blocks; // base number of blocks/dodges per turn
        int num_dodges;
        int num_blocks_bonus; // bonus ""
        int num_dodges_bonus;

        int armor_bash_bonus;
        int armor_cut_bonus;

        int speed_base; // only speed needs a base, the rest are assumed at 0 and calced off skills

        int speed_bonus;
        int dodge_bonus;
        int block_bonus;
        int hit_bonus;
        int bash_bonus;
        int cut_bonus;

        float bash_mult;
        float cut_mult;
        bool melee_quiet;

        int grab_resist;
        int throw_resist;

        virtual nc_color symbol_color();
        virtual nc_color basic_symbol_color();
        virtual char symbol();
        virtual bool is_symbol_highlighted();
};

#endif



