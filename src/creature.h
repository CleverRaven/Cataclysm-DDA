#ifndef _CREATURE_H_
#define _CREATURE_H_

#include "damage.h"
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
#include <set>

class game;
class effect;

class Creature
{
    public:
        // TODO: fill these in
        Creature();
        Creature(const Creature &rhs);

        virtual std::string disp_name() = 0; // displayname for Creature
        virtual std::string skin_name() = 0; // name of outer layer, e.g. "armor plates"

        virtual bool is_player() {
            return false;
        }
        virtual bool is_npc () {
            return false;
        }

        // Fake is used to mark non-real creatures used temporarally,
        // such as fake NPCs that fire weapons to simulate turrets.
        virtual bool is_fake ();
        virtual void set_fake (const bool fake_value);

        virtual void normalize(); // recreate the Creature from scratch
        virtual void reset(); // handle both reset steps. Call this function instead of reset_stats/bonuses
        virtual void reset_bonuses(); // reset the value of all bonus fields to 0
        virtual void reset_stats(); // prepare the Creature for the next turn
        virtual void die(Creature *killer) = 0;

        virtual int hit_roll() = 0;
        virtual int dodge_roll() = 0;

        // makes a single melee attack, with the currently equipped weapon
        virtual void melee_attack(Creature &t, bool allow_special) = 0; // Returns a damage

        // fires a projectile at target point from source point, with total_dispersion
        // dispersion. returns the rolled dispersion of the shot.
        virtual double projectile_attack(const projectile &proj, int sourcex, int sourcey,
                                         int targetx, int targety, double total_dispersion);
        // overloaded version, assume it comes from this Creature's position
        virtual double projectile_attack(const projectile &proj, int targetx, int targety,
                                         double total_dispersion);

        /*
        // instantly deals damage at the target point
        virtual int smite_attack(game* g, projectile &proj, int targetx, int targety,
                std::set<std::string>& proj_effects);
                */

        virtual int hit(Creature *source, body_part bphurt, int side,
                        int dam, int cut);


        // handles blocking of damage instance. mutates &dam
        virtual bool block_hit(body_part &bp_hit, int &side,
                               damage_instance &dam) = 0;

        // handles armor absorption (including clothing damage etc)
        // of damage instance. mutates &dam
        virtual void absorb_hit(body_part bp, int side,
                                damage_instance &dam) = 0;

        // TODO: this is just a shim so knockbacks work
        virtual void knock_back_from(int posx, int posy) = 0;

        // TODO: remove this function in favor of deal/apply_damage
        virtual void hurt(body_part bp, int side, int dam) = 0;

        // makes a melee attack against the creature
        // dealt_dam is overwritten with the values of the damage dealt
        // returns hit - dodge (>=0 = hit, <0 = miss)
        virtual int deal_melee_attack(Creature *source, int hitroll, bool crit,
                                      const damage_instance &d, dealt_damage_instance &dealt_dam);

        // makes a ranged projectile attack against the creature
        // dodgeable determines if the dodge stat applies or not, dodge is
        // reduced for ranged attacks
        virtual int deal_projectile_attack(Creature *source, double missed_by,
                                           const projectile &proj, dealt_damage_instance &dealt_dam);

        // deals the damage via an attack. Allows armor mitigation etc.
        // Most sources of external damage should use deal_damage
        // Mutates the damage_instance& object passed in to reflect the
        // post-mitigation object
        virtual dealt_damage_instance deal_damage(Creature *source, body_part bp, int side,
                                                  const damage_instance &d);
        // for each damage type, how much gets through and how much pain do we
        // accrue? mutates damage and pain
        virtual void deal_damage_handle_type(const damage_unit &du,
                                             body_part bp, int &damage, int &pain);
        // directly decrements the damage. ONLY handles damage, doesn't
        // increase pain, apply effects, etc
        virtual void apply_damage(Creature *source,
                                  body_part bp, int side, int amount) = 0;

        virtual bool digging();      // MF_DIGS or MF_CAN_DIG and diggable terrain
        virtual bool is_on_ground() = 0;
        virtual bool is_underwater() const = 0;
        virtual bool is_warm(); // is this creature warm, for IR vision, heat drain, etc
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

        virtual void process_effects(); // runs all the effects on the Creature

        // not-quite-stats, maybe group these with stats later
        virtual void mod_pain(int npain);
        virtual void mod_moves(int nmoves);

        /*
         * Get/set our killer, this is currently used exclusively to allow
         * mondeath effects to happen after death cleanup
         */
        virtual Creature *get_killer();

        /*
         * getters for stats - combat-related stats will all be held within
         * the Creature and re-calculated during every normalize() call
         */
        virtual int get_str() const;
        virtual int get_dex() const;
        virtual int get_per() const;
        virtual int get_int() const;

        virtual int get_str_base() const;
        virtual int get_dex_base() const;
        virtual int get_per_base() const;
        virtual int get_int_base() const;

        virtual int get_str_bonus() const;
        virtual int get_dex_bonus() const;
        virtual int get_per_bonus() const;
        virtual int get_int_bonus() const;

        virtual int get_num_blocks() const;
        virtual int get_num_dodges() const;
        virtual int get_num_blocks_bonus() const;
        virtual int get_num_dodges_bonus() const;

        virtual int get_env_resist(body_part bp);

        virtual int get_armor_bash(body_part bp);
        virtual int get_armor_cut(body_part bp);
        virtual int get_armor_bash_base(body_part bp);
        virtual int get_armor_cut_base(body_part bp);
        virtual int get_armor_bash_bonus();
        virtual int get_armor_cut_bonus();

        virtual int get_speed();
        virtual int get_dodge();
        virtual int get_hit();
        virtual m_size get_size()=0;

        virtual int get_speed_base();
        virtual int get_dodge_base();
        virtual int get_hit_base();
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

        virtual void set_speed_base(int nspeed);
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

        /*
         * Event handlers
         */

        virtual void on_gethit(Creature *source, body_part bp_hit,
                               damage_instance &dam);

        // innate stats, slowly move these to protected as we rewrite more of
        // the codebase
        int str_max, dex_max, per_max, int_max,
            str_cur, dex_cur, per_cur, int_cur;

        int moves, pain;

        void draw(WINDOW *w, int plx, int ply, bool inv);

        static void init_hit_weights();
    protected:
        Creature *killer; // whoever killed us. this should be NULL unless we are dead

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

        bool fake;

        Creature& operator= (const Creature& rhs);

        virtual nc_color symbol_color();
        virtual nc_color basic_symbol_color();
        virtual char symbol();
        virtual bool is_symbol_highlighted();


        //Hit weight work.
        static std::map<m_size, std::map<body_part, double> > default_hit_weights;

        typedef std::pair<body_part, double> weight_pair;

        struct weight_compare {
            bool operator() (const weight_pair &left, const weight_pair &right) { return left.second < right.second;}
        };

        body_part select_body_part(Creature *source, int hitroll);
};

#endif

