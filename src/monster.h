#ifndef MONSTER_H
#define MONSTER_H

#include "input.h"
#include "creature.h"
#include "player.h"
#include "enums.h"
#include <vector>

class map;
class game;
class item;
class monfaction;

typedef std::map< const monfaction*, std::set< int > > mfactions;

enum monster_attitude {
    MATT_NULL = 0,
    MATT_FRIEND,
    MATT_FPASSIVE,
    MATT_FLEE,
    MATT_IGNORE,
    MATT_FOLLOW,
    MATT_ATTACK,
    MATT_ZLAVE,
    NUM_MONSTER_ATTITUDES
};

class monster : public Creature, public JsonSerializer, public JsonDeserializer
{
        friend class editmap;
    public:
        monster();
        monster(mtype *t);
        monster( mtype *t, const tripoint &pos );
        monster(const monster &) = default;
        monster(monster &&) = default;
        virtual ~monster() override;
        monster &operator=(const monster &) = default;
        monster &operator=(monster &&) = default;

        virtual bool is_monster() const override
        {
            return true;
        }

        void poly(mtype *t);
        void update_check();
        void spawn( const int x, const int y ); // All this does is moves the monster to x,y,g->levz
        void spawn( const int x, const int y, const int z ); // As above, except with any z
        void spawn( const tripoint &p); // As above, but takes a tripoint argument
        m_size get_size() const override;
        int get_hp( hp_part ) const override
        {
            return hp;
        };
        int get_hp() const
        {
            return hp;
        }
        int get_hp_max( hp_part ) const override
        {
            return type->hp;
        };
        int get_hp_max() const
        {
            return type->hp;
        }
        std::string get_material() const override
        {
            return type->mat;
        };
        int hp_percentage() const override;

        // Access
        std::string name(unsigned int quantity = 1) const; // Returns the monster's formal name
        std::string name_with_armor() const; // Name, with whatever our armor is called
        // the creature-class versions of the above
        std::string disp_name(bool possessive = false) const override;
        std::string skin_name() const override;
        void get_HP_Bar(nc_color &color, std::string &text) const;
        void get_Attitude(nc_color &color, std::string &text) const;
        int print_info(WINDOW *w, int vStart, int vLines, int column) const override;

        // Information on how our symbol should appear
        nc_color basic_symbol_color() const override;
        nc_color symbol_color() const override;
        const std::string &symbol() const override;
        bool is_symbol_inverted() const;
        bool is_symbol_highlighted() const override;

        nc_color color_with_effects() const; // Color with fire, beartrapped, etc.
        // Inverts color if inv==true
        bool has_flag(const m_flag f) const override; // Returns true if f is set (see mtype.h)
        bool can_see() const;      // MF_SEES and no ME_BLIND
        bool can_hear() const;     // MF_HEARS and no ME_DEAF
        bool can_submerge() const; // MF_AQUATIC or MF_SWIMS or MF_NO_BREATH, and not MF_ELECTRONIC
        bool can_drown() const;    // MF_AQUATIC or MF_SWIMS or MF_NO_BREATHE or MF_FLIES
        bool digging() const override;      // MF_DIGS or MF_CAN_DIG and diggable terrain
        // Returns false if the monster is stunned, has 0 moves or otherwise wouldn't act this turn
        bool can_act() const;
        int sight_range( int light_level ) const override;
        using Creature::sees;
        bool made_of(std::string m) const; // Returns true if it's made of m
        bool made_of(phase_id p) const; // Returns true if its phase is p

        bool avoid_trap( const tripoint &pos, const trap &tr ) override;

        void load_legacy(std::stringstream &dump);
        void load_info(std::string data);

        using JsonSerializer::serialize;
        virtual void serialize(JsonOut &jsout) const override;
        using JsonDeserializer::deserialize;
        virtual void deserialize(JsonIn &jsin) override;

        void debug(player &u);      // Gives debug info

        tripoint move_target(); // Returns point at the end of the monster's current plans
        Creature *attack_target(); // Returns the creature at the end of plans (if hostile)

        // Movement
        void shift(int sx, int sy); // Shifts the monster to the appropriate submap
        // Updates current pos AND our plans
        bool wander(); // Returns true if we have no plans

        /**
         * Checks whether we can move to/through (x, y). This does not account for bashing.
         *
         * This is used in pathfinding and ONLY checks the terrain. It ignores players
         * and monsters, which might only block this tile temporarily.
         */
        bool can_move_to( const tripoint &p ) const;

        bool will_reach(int x, int y); // Do we have plans to get to (x, y)?
        int  turns_to_reach(int x, int y); // How long will it take?

        void set_dest( const tripoint &p, int &t ); // Go in a straight line to (x, y)
        // t determines WHICH Bresenham line

        /**
         * Set (x, y) as wander destination.
         *
         * This will cause the monster to slowly move towards the destination,
         * unless there is an overriding smell or plan.
         *
         * @param f The priority of the destination, as well as how long we should
         *          wander towards there.
         */
        void wander_to( const tripoint &p, int f ); // Try to get to (x, y), we don't know
        // the route.  Give up after f steps.

        // How good of a target is given creature (checks for visibility)
        float rate_target( Creature &c, int &bresen1, int &bresen2, float best, bool smart = false ) const;
        // Pass all factions to mon, so that hordes of same-faction mons
        // do not iterate over each other
        void plan(const mfactions &factions);
        void move(); // Actual movement
        void footsteps( const tripoint &p ); // noise made by movement
        void friendly_move();

        tripoint scent_move();
        tripoint wander_next();
        int calc_movecost( const tripoint &f, const tripoint &t ) const;

        /**
         * Attempt to move to p.
         *
         * If there's something blocking the movement, such as infinite move
         * costs at the target, an existing NPC or monster, this function simply
         * aborts and does nothing.
         *
         * @param force If this is set to true, the movement will happen even if
         *              there's currently something blocking the destination.
         *
         * @return true if movement successful, false otherwise
         */
        bool move_to( const tripoint &p, bool force = false );

        /**
         * Attack any enemies at the given location.
         *
         * Attacks only if there is a creature at the given location towards
         * we are hostile.
         *
         * @return true if something was attacked, false otherwise
         */
        bool attack_at( const tripoint &p );

        /**
         * Try to smash/bash/destroy your way through the terrain at p.
         *
         * @return true if we destroyed something, false otherwise.
         */
        bool bash_at( const tripoint &p );

        /** Returns innate monster bash skill, without calculating additional from helpers */
        int bash_skill();
        int bash_estimate();
        /** Returns ability of monster and any cooperative helpers to
         * bash the designated target.  **/
        int group_bash_skill( const tripoint &target );

        void stumble(bool moved);
        void knock_back_from( const tripoint &p ) override;

        // Combat
        bool is_fleeing(player &u) const; // True if we're fleeing
        monster_attitude attitude(player *u = NULL) const; // See the enum above
        Attitude attitude_to( const Creature &other ) const override;
        int morale_level(player &u); // Looks at our HP etc.
        void process_triggers(); // Process things that anger/scare us
        void process_trigger(monster_trigger trig, int amount); // Single trigger
        int trigger_sum(std::set<monster_trigger> *triggers) const;

        bool is_underwater() const override;
        bool is_on_ground() const override;
        bool is_warm() const override;
        bool has_weapon() const override;
        bool is_elec_immune() const override;
        bool is_dead_state() const override; // check if we should be dead or not

        void absorb_hit(body_part bp, damage_instance &dam) override;
        void dodge_hit(Creature *source, int hit_spread) override;
        bool block_hit(Creature *source, body_part &bp_hit, damage_instance &d) override;
        void melee_attack(Creature &p, bool allow_special = true, matec_id force_technique = "") override;
        virtual int deal_melee_attack(Creature *source, int hitroll) override;
        virtual int deal_projectile_attack(Creature *source, double missed_by,
                                           const projectile &proj, dealt_damage_instance &dealt_dam) override;
        virtual void deal_damage_handle_type(const damage_unit &du, body_part bp, int &damage, int &pain) override;
        void apply_damage(Creature *source, body_part bp, int amount) override;
        // create gibs/meat chunks/blood etc all over the place, does not kill, can be called on a dead monster.
        void explode();
        // Let the monster die and let its body explode into gibs
        void die_in_explosion( Creature *source );
        /**
         * Flat addition to the monsters @ref hp. This is not capped at the maximal hp!
         */
        void heal( int hp_delta );
        /**
         * Directly set the current @ref hp of the monster (not capped at the maximal hp).
         * You might want to use @ref heal / @ref apply_damage or @ref deal_damage instead.
         */
        void set_hp( int hp );

        /** Processes monster-specific effects effects before calling Creature::process_effects(). */
        virtual void process_effects() override;
        /** Processes effects which may prevent the monster from moving (bear traps, crushed, etc.).
         *  Returns false if movement is stopped. */
        virtual bool move_effects() override;
        /** Handles any monster-specific effect application effects before calling Creature::add_eff_effects(). */
        virtual void add_eff_effects(effect e, bool reduced) override;
        /** Performs any monster-specific modifications to the arguments before passing to Creature::add_effect(). */
        virtual void add_effect(efftype_id eff_id, int dur, body_part bp = num_bp, bool permanent = false,
                                int intensity = 0) override;

        virtual float power_rating() const override;

        int  get_armor_cut(body_part bp) const override;   // Natural armor, plus any worn armor
        int  get_armor_bash(body_part bp) const override;  // Natural armor, plus any worn armor
        int  get_dodge() const override;       // Natural dodge, or 0 if we're occupied
        int  get_melee() const override; // For determining attack skill when awarding dodge practice.
        int  hit_roll() const override;  // For the purposes of comparing to player::dodge_roll()
        int  dodge_roll() override;  // For the purposes of comparing to player::hit_roll()
        int  fall_damage() const; // How much a fall hurts us

        /** Resets a given special to its monster type cooldown value, an index of -1 does nothing. */
        void reset_special(int index);
        /** Resets a given special to a value between 0 and its monster type cooldown value.
          * An index of -1 does nothing. */
        void reset_special_rng(int index);
        /** Sets a given special to the given value, an index of -1 does nothing. */
        void set_special(int index, int time);

        void die(Creature *killer) override; //this is the die from Creature, it calls kill_mon
        void drop_items_on_death();

        // Other
        bool make_fungus();  // Makes this monster into a fungus version
        // Returns false if no such monster exists
        void make_friendly();
        /** Makes this monster an ally of the given monster. */
        void make_ally(monster* z);
        void add_item(item it);     // Add an item to inventory

        bool is_hallucination() const override;    // true if the monster isn't actually real

        field_id bloodType() const override;
        field_id gibType() const override;

        void add_msg_if_npc(const char *msg, ...) const override;
        void add_msg_if_npc(game_message_type type, const char *msg, ...) const override;
        void add_msg_player_or_npc(const char *, const char *npc_str, ...) const override;
        void add_msg_player_or_npc(game_message_type type, const char *, const char *npc_str, ...) const override;

        // TEMP VALUES
        tripoint wander_pos; // Wander destination - Just try to move in that direction
        int wandf;           // Urge to wander - Increased by sound, decrements each move
        std::vector<item> inv; // Inventory

        // DEFINING VALUES
        int def_chance;
        int friendly;
        int anger, morale;
        const monfaction *faction; // Our faction (species, for most monsters)
        int mission_id; // If we're related to a mission
        mtype *type;
        bool no_extra_death_drops;    // if true, don't spawn loot items as part of death
        bool no_corpse_quiet = false; //if true, monster dies quietly and leaves no corpse
        bool is_dead() const;
        bool made_footstep;
        std::string unique_name; // If we're unique
        bool hallucination;

        // level_change == true means "monster isn't spawned yet, don't update position in tracker"
        bool setpos( const int x, const int y );
        bool setpos( const int x, const int y, const int z, const bool level_change = false );
        bool setpos( const point &p, const bool level_change = false );
        bool setpos( const tripoint &p, const bool level_change = false );
        const tripoint &pos3() const override;
        inline int posx() const override
        {
            return position.x;
        }
        inline int posy() const override
        {
            return position.y;
        }
        inline int posz() const override
        {
            return position.z;
        }

        short ignoring;

        // Stair data.
        int staircount;

        // Ammunition if we use a gun.
        std::map<std::string, int> ammo;

        /**
         * Convert this monster into an item (see @ref mtype::revet_to_itype).
         * Only useful for robots and the like, the monster must have at least
         * a non-empty item id as revet_to_itype.
         */
        item to_item() const;
        /**
         * Initialize values like speed / hp from data of an item.
         * This applies to robotic monsters that are spawned by invoking an item (e.g. turret),
         * and to reviving monsters that spawn from a corpse.
         */
        void init_from_item( const item &itm );

        /** Sets the last time the monster was loaded to the current turn */
        void reset_last_load();

    private:
        int hp;
        std::vector<int> sp_timeout;
        std::vector <tripoint> plans;
        tripoint position;
        int last_loaded; //time the monster was last loaded
        bool dead;
        /** Attack another monster */
        void hit_monster(monster &other);
        /** Legacy loading logic for monsters that are packing ammo. **/
        void normalize_ammo( const int old_ammo );

    protected:
        void store(JsonOut &jsout) const;
        void load(JsonObject &jsin);
};

#endif
