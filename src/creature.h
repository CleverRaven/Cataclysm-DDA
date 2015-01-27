#ifndef CREATURE_H
#define CREATURE_H

#include "damage.h"
#include "pldata.h"
#include "skill.h"
#include "bionics.h"
#include "json.h"
#include "effect.h"
#include "bodypart.h"
#include "mtype.h"
#include "output.h"
#include "cursesdef.h" // WINDOW

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
        virtual bool is_monster() const
        {
            return false;
        }

        /** Returns true for non-real Creatures used temporarily; i.e. fake NPC's used for turret fire. */
        virtual bool is_fake () const;
        /** Sets a Creature's fake boolean. */
        virtual void set_fake (const bool fake_value);

        /** Recreates the Creature from scratch. */
        virtual void normalize();
        /** Processes effects and bonuses and allocates move points based on speed. */
        virtual void process_turn();
        /** Handles both reset steps. Should be called instead of each individual reset function generally. */
        virtual void reset();
        /** Resets the value of all bonus fields to 0. */
        virtual void reset_bonuses();
        /** Resets creature stats to normal levels for the start of each turn. Should be idempotent. */
        virtual void reset_stats();
        
        /** Empty function. Should always be overwritten by the appropriate player/NPC/monster version. */
        virtual void die(Creature *killer) = 0;

        /** Should always be overwritten by the appropriate player/NPC/monster version, return 0 just in case. */
        virtual int hit_roll() const = 0;
        virtual int dodge_roll() = 0;

        /**
         * Simplified attitude towards any creature:
         * hostile - hate, want to kill, etc.
         * friendly - avoid harming it, maybe even help.
         * neutral - anything between.
         */
        enum Attitude {
            A_HOSTILE,
            A_NEUTRAL,
            A_FRIENDLY,
        };
        /**
         * Attitude (of this creature) towards another creature. This might not be symmetric.
         */
        virtual Attitude attitude_to( const Creature &other ) const = 0;

        /**
         * The functions check whether this creature can see the target.
         * The target may either be another creature (critter), or a specific point on the map.
         *
         * The bresenham_slope parameter is only set when the target is actually visible. Its
         * value must be passed to @ref line_to to get a line from this creature to the target
         * which will pass through transparent terrain only. Using a different value for line_to
         * may result in a line that passes through opaque terrain.
         *
         * Different creatures types are supposed to only implement the two virtual functions.
         * The other functions are here to give the callers more freedom, they simply forward
         * to one of the virtual functions.
         *
         * The function that take another creature as input should check visibility of that creature
         * (e.g. not digging, or otherwise invisible). They must than check whether the location of
         * the other monster is visible.
         */
        /*@{*/
        virtual bool sees( const Creature &critter, int &bresenham_slope ) const;
        bool sees( const Creature &critter ) const;
        bool sees( int cx, int cy, int &bresenham_slope ) const;
        bool sees( int tx, int ty ) const;
        virtual bool sees( point t, int &bresenham_slope ) const;
        bool sees( point t ) const;
        /*@}*/

        /**
         * How far the creature sees under the given light. Places outside this range can
         * @param light_level See @ref game::light_level.
         */
        virtual int sight_range( int light_level ) const = 0;

        /** Returns an approximation of the creature's strength. Should always be overwritten by
         *  the appropriate player/NPC/monster function. */
        virtual float power_rating() const = 0;
        /**
         * For fake-players (turrets, mounted turrets) this functions
         * chooses a target. This is for creatures that are friendly towards
         * the player and therefor choose a target that is hostile
         * to the player.
         * @param pos Position of the fake-player
         * @param range The maximal range to look for monsters, anything
         * outside of that range is ignored.
         * @param boo_hoo The number of targets that have been skipped
         * because the player is in the way.
         * @param area The area of effect of the projectile aimed.
         */
        Creature *auto_find_hostile_target( int range, int &boo_hoo, int area = 0);

        /** Make a single melee attack with the currently equipped weapon against the targeted
         *  creature. Should always be overwritten by the appropriate player/NPC/monster function. */
        virtual void melee_attack(Creature &t, bool allow_special,
                                  matec_id technique) = 0;

        /** Fires a projectile at the target point from the source point with total_dispersion
         *  dispersion. Returns the rolled dispersion of the shot. */
        virtual double projectile_attack(const projectile &proj, int sourcex, int sourcey,
                                         int targetx, int targety, double total_dispersion);
        /** Overloaded version that assumes the projectile comes from this Creature's postion. */
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
        // returns true if health is zero or otherwise should be dead
        virtual bool is_dead_state() const = 0;

        virtual int posx() const = 0;
        virtual int posy() const = 0;
        virtual const point &pos() const = 0;

        struct compare_by_dist_to_point {
            point center;
            // Compare the two creatures a and b by their distance to a fixed center point.
            // The nearer creature is considered smaller and sorted first.
            bool operator()( const Creature *a, const Creature *b ) const;
        };

        /** Processes move stopping effects. Returns false if movement is stopped. */
        virtual bool move_effects();

        /** Handles effect application effects. */
        virtual void add_eff_effects(effect e, bool reduced);

        /** Adds or modifies an effect. If intensity is given it will set the effect intensity
            to the given value, or as close as max_intensity values permit. */
        virtual void add_effect(efftype_id eff_id, int dur, body_part bp = num_bp, bool permanent = false,
                                int intensity = 0);
        /** Gives chance to save via environmental resist, returns false if resistance was successful. */
        bool add_env_effect(efftype_id eff_id, body_part vector, int strength, int dur,
                            body_part bp = num_bp, bool permanent = false, int intensity = 1);
        /** Removes a listed effect, adding the removal memorial log if needed. bp = num_bp means to remove
         *  all effects of a given type, targeted or untargeted. Returns true if anything was removed. */
        bool remove_effect(efftype_id eff_id, body_part bp = num_bp);
        /** Remove all effects. */
        void clear_effects();
        /** Check if creature has the matching effect. bp = num_bp means to check if the Creature has any effect
         *  of the matching type, targeted or untargeted. */
        bool has_effect(efftype_id eff_id, body_part bp = num_bp) const;
        /** Return the effect that matches the given arguments exactly. */
        effect get_effect(efftype_id eff_id, body_part bp = num_bp) const;
        /** Returns the duration of the matching effect. Returns 0 if effect doesn't exist. */
        int get_effect_dur(efftype_id eff_id, body_part bp = num_bp) const;
        /** Returns the intensity of the matching effect. Returns 0 if effect doesn't exist. */
        int get_effect_int(efftype_id eff_id, body_part bp = num_bp) const;

        // Methods for setting/getting misc key/value pairs.
        void set_value( const std::string key, const std::string value );
        void remove_value( const std::string key );
        std::string get_value( const std::string key ) const;

        /** Processes through all the effects on the Creature. */
        virtual void process_effects();

        /** Returns true if the player has the entered trait, returns false for non-humans */
        virtual bool has_trait(const std::string &flag) const;

        /** Handles health fluctuations over time */
        virtual void update_health(int base_threshold = 0);

        // not-quite-stats, maybe group these with stats later
        virtual void mod_pain(int npain);
        virtual void mod_moves(int nmoves);
        virtual void set_moves(int nmoves);

        virtual bool in_sleep_state() const;

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
        virtual int hp_percentage() const = 0;
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
        virtual bool uncanny_dodge()
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

        // Storing body_part as an int to make things easier for hash and JSON
        std::unordered_map<std::string, std::unordered_map<body_part, effect, std::hash<int>>> effects;
        // Miscellaneous key/value pairs.
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
         * This function replaces the "<npcname>" substring with the provided NPC name.
         *
         * Its purpose is to avoid repeated code and improve source readability / maintainability.
         *
         */
        inline std::string replace_with_npc_name(std::string str, std::string name) const
        {
            size_t offset = str.find("<npcname>");
            if (offset != std::string::npos) {
                str.replace(offset, 9, name);
                if (offset == 0 && !str.empty()) {
                    capitalize_letter(str, 0);
                }
            }
            return str;
        }

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
