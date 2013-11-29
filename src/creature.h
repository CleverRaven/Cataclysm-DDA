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

        void normalize(game* g); // recreate the creature from scratch
        void reset(game* g); // prepare the creature for the next turn

        // should replace both player.add_disease and monster.add_effect
        void add_effect(std::string eff_id, int dur);
        void has_effect(std::string eff_id);


        // getters for stats - combat-related stats will all be held within
        // the creature and re-calculated during every normalize() call

        int get_str();
        int get_dex();
        int get_per();
        int get_int();

        int get_str_bonus();
        int get_dex_bonus();
        int get_per_bonus();
        int get_int_bonus();

        int get_num_blocks();
        int get_num_dodges();
        int get_num_blocks_bonus();
        int get_num_dodges_bonus();

        int get_arm_bash_bonus();
        int get_arm_cut_bonus();

        int get_speed();
        int get_dodge();

        int get_speed_bonus();
        int get_dodge_bonus();
        int get_block_bonus();
        int get_hit_bonus();
        int get_bash_bonus();
        int get_cut_bonus();

        float get_bash_mult();
        float get_cut_mult();

        bool get_melee_quiet();
        int get_throw_resist();

        // setters for stat boni
        void set_str_bonus(int nstr);
        void set_dex_bonus(int ndex);
        void set_per_bonus(int nper);
        void set_int_bonus(int nint);
        void mod_str_bonus(int nstr);
        void mod_dex_bonus(int ndex);
        void mod_per_bonus(int nper);
        void mod_int_bonus(int nint);

        void set_num_blocks_bonus(int nblocks);
        void set_num_dodges_bonus(int ndodges);

        void set_arm_bash_bonus(int nbasharm);
        void set_arm_cut_bonus(int ncutarm);

        void set_speed_bonus(int nspeed);
        void set_dodge_bonus(int ndodge);
        void set_block_bonus(int nblock);
        void set_hit_bonus(int nhit);
        void set_bash_bonus(int nbash);
        void set_cut_bonus(int ncut);

        void set_bash_mult(float nbashmult);
        void set_cut_mult(float ncutmult);

        void set_melee_quiet(bool nquiet);
        void set_throw_resist(int nthrowres);

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



