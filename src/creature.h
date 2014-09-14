#ifndef _CREATURE_H_
#define _CREATURE_H_

#include "damage.h"
#include "pldata.h"
#include "skill.h"
#include "bionics.h"
#include "json.h"
#include "effect.h"
#include "bodypart.h"
#include "mtype.h"
#include "output.h"
#include <stdlib.h>
#include <string>
#include <unordered_map>

class game;
class JsonObject;
class JsonOut;

class Creature
{
    public:
        virtual ~Creature();

        virtual std::string disp_name(bool possessive = false) const = 0; // displayname for Creature
        virtual std::string skin_name() const = 0; // name of outer layer, e.g. "armor plates"

        virtual bool is_player() const
        {
            return false;
        }
        virtual bool is_npc () const
        {
            return false;
        }

        // Fake is used to mark non-real creatures used temporarally,
        // such as fake NPCs that fire weapons to simulate turrets.
        virtual bool is_fake () const;
        virtual void set_fake (const bool fake_value);

        virtual void normalize(); // recreate the Creature from scratch
        virtual void
        process_turn(); // handles long-term non-idempotent updates to creature state (e.g. moves += speed, bionics hunger costs)
        virtual void reset(); // handle both reset steps. Call this function instead of reset_stats/bonuses
        virtual void reset_bonuses(); // reset the value of all bonus fields to 0
        virtual void reset_stats(); // prepare the Creature for the next turn. Should be idempotent
        virtual void die(Creature *killer) = 0;

        virtual int hit_roll() const = 0;
        virtual int dodge_roll() = 0;

        // makes a single melee attack, with the currently equipped weapon
        virtual void melee_attack(Creature &t, bool allow_special,
                                  matec_id technique) = 0; // Returns a damage

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

        // handles dodges and misses, allowing triggering of martial arts counter
        virtual void dodge_hit(Creature *source, int hit_spread) = 0;

        // handles blocking of damage instance. mutates &dam
        virtual bool block_hit(Creature *source, body_part &bp_hit,
                               damage_instance &dam) = 0;

        // handles armor absorption (including clothing damage etc)
        // of damage instance. mutates &dam
        virtual void absorb_hit(body_part bp, damage_instance &dam) = 0;

        // TODO: this is just a shim so knockbacks work
        virtual void knock_back_from(int posx, int posy) = 0;

        // begins a melee attack against the creature
        // returns hit - dodge (>=0 = hit, <0 = miss)
        virtual int deal_melee_attack(Creature *source, int hitroll);

        // completes a melee attack against the creature
        // dealt_dam is overwritten with the values of the damage dealt
        virtual void deal_melee_hit(Creature *source, int hit_spread, bool crit,
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
        virtual dealt_damage_instance deal_damage(Creature *source, body_part bp,
                const damage_instance &d);
        // for each damage type, how much gets through and how much pain do we
        // accrue? mutates damage and pain
        virtual void deal_damage_handle_type(const damage_unit &du,
                                             body_part bp, int &damage, int &pain);
        // directly decrements the damage. ONLY handles damage, doesn't
        // increase pain, apply effects, etc
        virtual void apply_damage(Creature *source, body_part bp, int amount) = 0;

        virtual bool digging() const;      // MF_DIGS or MF_CAN_DIG and diggable terrain
        virtual bool is_on_ground() const = 0;
        virtual bool is_underwater() const = 0;
        virtual bool is_warm() const; // is this creature warm, for IR vision, heat drain, etc
        virtual bool has_weapon() const = 0;
        virtual bool is_hallucination() const = 0;
        // returns true iff health is zero or otherwise should be dead
        virtual bool is_dead_state() const = 0;

        // xpos and ypos, because posx/posy are used as public variables in
        // player.cpp and therefore referenced everywhere
        virtual int xpos() const = 0;
        virtual int ypos() const = 0;
        virtual point pos() const = 0;

        // should replace both player.add_disease and monster.add_effect
        // these are nonvirtual since otherwise they can't be accessed with
        // the old add_effect
        void add_effect(efftype_id eff_id, int dur, int intensity = 1, bool permanent = false);
        bool add_env_effect(efftype_id eff_id, body_part vector, int strength, int dur,
                            int intensity = 1, bool permanent =
                                false); // gives chance to save via env resist, returns if successful
        void remove_effect(efftype_id eff_id);
        void clear_effects(); // remove all effects
        bool has_effect(efftype_id eff_id) const;

        // Methods for setting/getting misc key/value pairs.
        void set_value( const std::string key, const std::string value );
        void remove_value( const std::string key );
        std::string get_value( const std::string key ) const;

        virtual void process_effects(); // runs all the effects on the Creature

        /** Handles health fluctuations over time */
        virtual void update_health(int base_threshold = 0);

        // not-quite-stats, maybe group these with stats later
        virtual void mod_pain(int npain);
        virtual void mod_moves(int nmoves);

        /*
         * Get/set our killer, this is currently used exclusively to allow
         * mondeath effects to happen after death cleanup
         */
        virtual Creature *get_killer() const;

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

        virtual int get_healthy() const;
        virtual int get_healthy_mod() const;

        virtual int get_num_blocks() const;
        virtual int get_num_dodges() const;
        virtual int get_num_blocks_bonus() const;
        virtual int get_num_dodges_bonus() const;

        virtual int get_env_resist(body_part bp) const;

        virtual int get_armor_bash(body_part bp) const;
        virtual int get_armor_cut(body_part bp) const;
        virtual int get_armor_bash_base(body_part bp) const;
        virtual int get_armor_cut_base(body_part bp) const;
        virtual int get_armor_bash_bonus() const;
        virtual int get_armor_cut_bonus() const;

        virtual int get_speed() const;
        virtual int get_dodge() const;
        virtual int get_melee() const;
        virtual int get_hit() const;
        virtual m_size get_size() const = 0;
        virtual int get_hp( hp_part bp = num_hp_parts ) const = 0;
        virtual int get_hp_max( hp_part bp = num_hp_parts ) const = 0;
        virtual std::string get_material() const
        {
            return "flesh";
        }
        virtual field_id bloodType () const = 0;
        virtual field_id gibType () const = 0;
        // TODO: replumb this to use a std::string along with monster flags.
        virtual bool has_flag( const m_flag ) const
        {
            return false;
        };

        virtual int get_speed_base() const;
        virtual int get_dodge_base() const;
        virtual int get_hit_base() const;
        virtual int get_speed_bonus() const;
        virtual int get_dodge_bonus() const;
        virtual int get_block_bonus() const;
        virtual int get_hit_bonus() const;
        virtual int get_bash_bonus() const;
        virtual int get_cut_bonus() const;

        virtual float get_bash_mult() const;
        virtual float get_cut_mult() const;

        virtual bool get_melee_quiet() const;
        virtual int get_grab_resist() const;
        virtual int get_throw_resist() const;

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
        virtual void mod_stat( std::string stat, int modifier );

        virtual void set_healthy(int nhealthy);
        virtual void set_healthy_mod(int nhealthy_mod);
        virtual void mod_healthy(int nhealthy);
        virtual void mod_healthy_mod(int nhealthy_mod);

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

        virtual int weight_capacity() const;

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

        void draw(WINDOW *w, int plx, int ply, bool inv) const;
        /**
         * Write information about this creature.
         * @param w the window to print the text into.
         * @param vStart vertical start to print, that means the first line to print.
         * @param vLines number of lines to print at most (printing less is fine).
         * @param column horizontal start to print (column), horizontal end is
         * one character before  the right border of the window (to keep the border).
         * @return The line just behind the last printed line, that means multiple calls
         * to this can be stacked, the return value is acceptable as vStart for the next
         * call without creating empty lines or overwriting lines.
         */
        virtual int print_info(WINDOW *w, int vStart, int vLines, int column) const = 0;

        // Message related stuff
        virtual void add_msg_if_player(const char *, ...) const {};
        virtual void add_msg_if_player(game_message_type, const char *, ...) const {};
        virtual void add_msg_if_npc(const char *, ...) const {};
        virtual void add_msg_if_npc(game_message_type, const char *, ...) const {};
        virtual void add_msg_player_or_npc(const char *, const char *, ...) const {};
        virtual void add_msg_player_or_npc(game_message_type, const char *, const char *, ...) const {};

        virtual void add_memorial_log(const char *, const char *, ...) {};

        virtual nc_color symbol_color() const;
        virtual nc_color basic_symbol_color() const;
        virtual const std::string &symbol() const;
        virtual bool is_symbol_highlighted() const;

    protected:
        Creature *killer; // whoever killed us. this should be NULL unless we are dead

        std::unordered_map<std::string, effect> effects;
        // Miscelaneous key/value pairs.
        std::unordered_map<std::string, std::string> values;

        // used for innate bonuses like effects. weapon bonuses will be
        // handled separately

        int str_bonus;
        int dex_bonus;
        int per_bonus;
        int int_bonus;

        int healthy; //How healthy the creature is, currently only used by players
        int healthy_mod;

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

        Creature();
        Creature(const Creature &) = default;
        Creature(Creature &&) = default;
        Creature &operator=(const Creature &) = default;
        Creature &operator=(Creature &&) = default;

        body_part select_body_part(Creature *source, int hit_roll);

        /**
         * These two functions are responsible for storing and loading the members
         * of this class to/from json data.
         * All classes that inherit from this class should implement their own
         * version of these functions. They are not virtual on purpose, as it's
         * not needed.
         * Every implementation of those functions should a) call the same function
         * with the same parameters of the super class and b) store/load their own
         * members, but *not* members of any sub or super class.
         *
         * The functions are (on purpose) not part of the json
         * serialize/deserialize system (defined in json.h).
         * The Creature class is incomplete, there won't be any instance of that
         * class alone, but there will be instances of sub classes (e.g. player,
         * monster).
         * Those (complete) classes implement the deserialize/serialize interface
         * of json. That way only one json object per npc/player/monster instance
         * is created (inside of the serialize function).
         * E.g. player::serialize() looks like this:
         * <code>json.start_object();store(json);json.end_object()</code>
         * player::store than stores the members of the player class, and it calls
         * Character::store, which stores the members of the Character class and calls
         * Creature::store, which stores the members of the Creature class.
         * All data goes into the single json object created in player::serialize.
         */
        // Store data of *this* class in the stream
        void store(JsonOut &jsout) const;
        // Load creature data from the given json object.
        void load(JsonObject &jsin);
};

#endif

