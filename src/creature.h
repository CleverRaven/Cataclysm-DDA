#ifndef _CREATURE_H_
#define _CREATURE_H_

#include "pldata.h"
#include "skill.h"
#include "bionics.h"
#include "json.h"
#include "effect.h"
#include <string>
#include <vector>


class game;
class effect;

class creature
{
    public:
        // TODO: fill these in
        creature();
        creature(const creature & rhs);

        virtual std::string disp_name() = 0; // displayname for creature
        virtual std::string skin_name() = 0; // name of outer layer, e.g. "armor plates"

        virtual int dodge_roll(game* g) = 0;

        virtual int hit_creature(game *g, creature &t, bool allow_grab) = 0; // Returns a damage

        virtual int hit (game *g, body_part bphurt, int side,
                int dam, int cut) = 0;

        virtual bool block_hit(game *g, creature &t, body_part &bp_hit, int &side,
            int &bash_dam, int &cut_dam, int &stab_dam) = 0;

        virtual bool is_on_ground() = 0;
        virtual bool has_weapon() = 0;

        // xpos and ypos, because posx/posy are used as public variables in
        // player.cpp and therefore referenced everywhere
        virtual int xpos() = 0;
        virtual int ypos() = 0;

        // these also differ between player and monster (player takes
        // body_part) but we will use this for now
        // TODO: change uses of this to get_arm_bash/get_arm_cut
        virtual int armor_bash() = 0;
        virtual int armor_cut() = 0;

        virtual bool is_player() { return false; }

        virtual void normalize(game* g); // recreate the creature from scratch
        virtual void reset(game* g); // prepare the creature for the next turn

        // should replace both player.add_disease and monster.add_effect
        // these are nonvirtual since otherwise they can't be accessed with
        // the old add_effect
        void add_effect(efftype_id eff_id, int dur);
        void remove_effect(efftype_id eff_id);
        void clear_effects(); // remove all effects
        bool has_effect(efftype_id eff_id);

        virtual void process_effects(game* g); // runs all the effects on the creature

        // getters for stats - combat-related stats will all be held within
        // the creature and re-calculated during every normalize() call

        virtual int get_str();
        virtual int get_dex();
        virtual int get_per();
        virtual int get_int();

        virtual int get_str_bonus();
        virtual int get_dex_bonus();
        virtual int get_per_bonus();
        virtual int get_int_bonus();

        virtual int get_num_blocks();
        virtual int get_num_dodges();
        virtual int get_num_blocks_bonus();
        virtual int get_num_dodges_bonus();

        virtual int get_arm_bash_bonus();
        virtual int get_arm_cut_bonus();

        virtual int get_speed();
        virtual int get_dodge();

        virtual int get_speed_bonus();
        virtual int get_dodge_bonus();
        virtual int get_block_bonus();
        virtual int get_hit_bonus();
        virtual int get_bash_bonus();
        virtual int get_cut_bonus();

        virtual float get_bash_mult();
        virtual float get_cut_mult();

        virtual bool get_melee_quiet();
        virtual int get_throw_resist();

        // setters for stat boni
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

        virtual void set_arm_bash_bonus(int nbasharm);
        virtual void set_arm_cut_bonus(int ncutarm);

        virtual void set_speed_bonus(int nspeed);
        virtual void set_dodge_bonus(int ndodge);
        virtual void set_block_bonus(int nblock);
        virtual void set_hit_bonus(int nhit);
        virtual void set_bash_bonus(int nbash);
        virtual void set_cut_bonus(int ncut);

        virtual void set_bash_mult(float nbashmult);
        virtual void set_cut_mult(float ncutmult);

        virtual void set_melee_quiet(bool nquiet);
        virtual void set_throw_resist(int nthrowres);

        // innate stats, slowly move these to protected as we rewrite more of
        // the codebase
        int str_max;
        int dex_max;
        int per_max;
        int int_max;
        int str_cur;
        int dex_cur;
        int per_cur;
        int int_cur;

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

        int arm_bash_bonus;
        int arm_cut_bonus;

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
        int throw_resist;


};

#endif



