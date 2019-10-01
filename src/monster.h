#pragma once
#ifndef MONSTER_H
#define MONSTER_H

#include <climits>
#include <cstddef>
#include <bitset>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "calendar.h"
#include "character_id.h"
#include "creature.h"
#include "enums.h"
#include "bodypart.h"
#include "color.h"
#include "cursesdef.h"
#include "damage.h"
#include "item.h"
#include "mtype.h"
#include "optional.h"
#include "pldata.h"
#include "type_id.h"
#include "units.h"
#include "point.h"

class JsonObject;
class JsonIn;
class JsonOut;
class player;
class Character;
class effect;
struct dealt_projectile_attack;
struct pathfinding_settings;
struct trap;

enum class mon_trigger;

class monster;

class mon_special_attack
{
    public:
        int cooldown = 0;
        bool enabled = true;

        void serialize( JsonOut &json ) const;
        // deserialize inline in monster::load due to backwards/forwards compatibility concerns
};

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

enum monster_effect_cache_fields {
    MOVEMENT_IMPAIRED = 0,
    FLEEING,
    VISION_IMPAIRED,
    NUM_MEFF
};

enum monster_horde_attraction {
    MHA_NULL = 0,
    MHA_ALWAYS,
    MHA_LARGE,
    MHA_OUTDOORS,
    MHA_OUTDOORS_AND_LARGE,
    MHA_NEVER,
    NUM_MONSTER_HORDE_ATTRACTION
};

class monster : public Creature
{
        friend class editmap;
    public:
        monster();
        monster( const mtype_id &id );
        monster( const mtype_id &id, const tripoint &pos );
        monster( const monster & ) ;
        monster( monster && );
        ~monster() override;
        monster &operator=( const monster & );
        monster &operator=( monster && );

        bool is_monster() const override {
            return true;
        }

        void poly( const mtype_id &id );
        bool can_upgrade();
        void hasten_upgrade();
        void try_upgrade( bool pin_time );
        void try_reproduce();
        void try_biosignature();
        void spawn( const tripoint &p );
        m_size get_size() const override;
        units::mass get_weight() const override;
        units::volume get_volume() const;
        int get_hp( hp_part ) const override;
        int get_hp() const override;
        int get_hp_max( hp_part ) const override;
        int get_hp_max() const override;
        int hp_percentage() const override;

        // Access
        std::string get_name() const override;
        std::string name( unsigned int quantity = 1 ) const; // Returns the monster's formal name
        std::string name_with_armor() const; // Name, with whatever our armor is called
        // the creature-class versions of the above
        std::string disp_name( bool possessive = false ) const override;
        std::string skin_name() const override;
        void get_HP_Bar( nc_color &color, std::string &text ) const;
        std::pair<std::string, nc_color> get_attitude() const;
        int print_info( const catacurses::window &w, int vStart, int vLines, int column ) const override;

        // Information on how our symbol should appear
        nc_color basic_symbol_color() const override;
        nc_color symbol_color() const override;
        const std::string &symbol() const override;
        bool is_symbol_highlighted() const override;

        nc_color color_with_effects() const; // Color with fire, beartrapped, etc.

        std::string extended_description() const override;
        // Inverts color if inv==true
        bool has_flag( m_flag f ) const override; // Returns true if f is set (see mtype.h)
        bool can_see() const;      // MF_SEES and no ME_BLIND
        bool can_hear() const;     // MF_HEARS and no ME_DEAF
        bool can_submerge() const; // MF_AQUATIC or MF_SWIMS or MF_NO_BREATH, and not MF_ELECTRONIC
        bool can_drown() const;    // MF_AQUATIC or MF_SWIMS or MF_NO_BREATHE or MF_FLIES
        bool digging() const override;      // MF_DIGS or MF_CAN_DIG and diggable terrain
        // Returns false if the monster is stunned, has 0 moves or otherwise wouldn't act this turn
        bool can_act() const;
        int sight_range( int light_level ) const override;
        bool made_of( const material_id &m ) const override; // Returns true if it's made of m
        bool made_of_any( const std::set<material_id> &ms ) const override;
        bool made_of( phase_id p ) const; // Returns true if its phase is p

        bool avoid_trap( const tripoint &pos, const trap &tr ) const override;

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );

        tripoint move_target(); // Returns point at the end of the monster's current plans
        Creature *attack_target(); // Returns the creature at the end of plans (if hostile)

        // Movement
        void shift( const point &sm_shift ); // Shifts the monster to the appropriate submap
        void set_goal( const tripoint &p );
        // Updates current pos AND our plans
        bool wander(); // Returns true if we have no plans

        /**
         * Checks whether we can move to/through p. This does not account for bashing.
         *
         * This is used in pathfinding and ONLY checks the terrain. It ignores players
         * and monsters, which might only block this tile temporarily.
         */
        bool can_move_to( const tripoint &p ) const;

        bool will_reach( const point &p ); // Do we have plans to get to (x, y)?
        int  turns_to_reach( const point &p ); // How long will it take?

        // Go in a straight line to p
        void set_dest( const tripoint &p );
        // Reset our plans, we've become aimless.
        void unset_dest();

        /**
         * Set p as wander destination.
         *
         * This will cause the monster to slowly move towards the destination,
         * unless there is an overriding smell or plan.
         *
         * @param p Destination of monster's wanderings
         * @param f The priority of the destination, as well as how long we should
         *          wander towards there.
         */
        void wander_to( const tripoint &p, int f ); // Try to get to (x, y), we don't know
        // the route.  Give up after f steps.

        // How good of a target is given creature (checks for visibility)
        float rate_target( Creature &c, float best, bool smart = false ) const;
        void plan();
        void move(); // Actual movement
        void footsteps( const tripoint &p ); // noise made by movement
        void shove_vehicle( const tripoint &remote_destination,
                            const tripoint &nearby_destination ); // shove vehicles out of the way
        // check if a monster at a position will drown and kill it if necessary
        // returns true if the monster dies
        // chance is the one_in( chance ) that the monster will drown
        bool die_if_drowning( const tripoint &at_pos, int chance = 1 );


        tripoint scent_move();
        int calc_movecost( const tripoint &f, const tripoint &t ) const;
        int calc_climb_cost( const tripoint &f, const tripoint &t ) const;

        bool is_immune_field( field_type_id fid ) const override;

        /**
         * Attempt to move to p.
         *
         * If there's something blocking the movement, such as infinite move
         * costs at the target, an existing NPC or monster, this function simply
         * aborts and does nothing.
         *
         * @param p Destination of movement
         * @param force If this is set to true, the movement will happen even if
         *              there's currently something blocking the destination.
         *
         * @param stagger_adjustment is a multiplier for move cost to compensate for staggering.
         *
         * @return true if movement successful, false otherwise
         */
        bool move_to( const tripoint &p, bool force = false, float stagger_adjustment = 1.0 );

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

        /**
         * Try to push away whatever occupies p, then step in.
         * May recurse and try to make the creature at p push further.
         *
         * @param p Location of pushed object
         * @param boost A bonus on the roll to represent a horde pushing from behind
         * @param depth Number of recursions so far
         *
         * @return True if we managed to push something and took its place, false otherwise.
         */
        bool push_to( const tripoint &p, int boost, size_t depth );

        /** Returns innate monster bash skill, without calculating additional from helpers */
        int bash_skill();
        int bash_estimate();
        /** Returns ability of monster and any cooperative helpers to
         * bash the designated target.  **/
        int group_bash_skill( const tripoint &target );

        void stumble();
        void knock_back_to( const tripoint &to ) override;

        // Combat
        bool is_fleeing( player &u ) const; // True if we're fleeing
        monster_attitude attitude( const Character *u = nullptr ) const; // See the enum above
        Attitude attitude_to( const Creature &other ) const override;
        void process_triggers(); // Process things that anger/scare us

        bool is_underwater() const override;
        bool is_on_ground() const override;
        bool is_warm() const override;
        bool has_weapon() const override;
        bool is_dead_state() const override; // check if we should be dead or not
        bool is_elec_immune() const override;
        bool is_immune_effect( const efftype_id & ) const override;
        bool is_immune_damage( damage_type ) const override;

        void absorb_hit( body_part bp, damage_instance &dam ) override;
        bool block_hit( Creature *source, body_part &bp_hit, damage_instance &d ) override;
        void melee_attack( Creature &target );
        void melee_attack( Creature &target, float accuracy );
        void melee_attack( Creature &p, bool ) = delete;
        void deal_projectile_attack( Creature *source, dealt_projectile_attack &attack,
                                     bool print_messages = true ) override;
        void deal_damage_handle_type( const damage_unit &du, body_part bp, int &damage,
                                      int &pain ) override;
        void apply_damage( Creature *source, body_part bp, int dam,
                           bool bypass_med = false ) override;
        // create gibs/meat chunks/blood etc all over the place, does not kill, can be called on a dead monster.
        void explode();
        // Let the monster die and let its body explode into gibs
        void die_in_explosion( Creature *source );
        /**
         * Flat addition to the monsters @ref hp. If `overheal` is true, this is not capped by max hp.
         * Returns actually healed hp.
         */
        int heal( int delta_hp, bool overheal = false );
        /**
         * Directly set the current @ref hp of the monster (not capped at the maximal hp).
         * You might want to use @ref heal / @ref apply_damage or @ref deal_damage instead.
         */
        void set_hp( int hp );

        /** Processes monster-specific effects before calling Creature::process_effects(). */
        void process_effects() override;

        /** Returns true if the monster has its movement impaired */
        bool movement_impaired();
        /** Processes effects which may prevent the monster from moving (bear traps, crushed, etc.).
         *  Returns false if movement is stopped. */
        bool move_effects( bool attacking ) override;
        /** Performs any monster-specific modifications to the arguments before passing to Creature::add_effect(). */
        void add_effect( const efftype_id &eff_id, time_duration dur, body_part bp = num_bp,
                         bool permanent = false,
                         int intensity = 0, bool force = false, bool deferred = false ) override;
        /** Returns a std::string containing effects for descriptions */
        std::string get_effect_status() const;

        float power_rating() const override;
        float speed_rating() const override;

        int get_worn_armor_val( damage_type dt ) const;
        int  get_armor_cut( body_part bp ) const override; // Natural armor, plus any worn armor
        int  get_armor_bash( body_part bp ) const override; // Natural armor, plus any worn armor
        int  get_armor_type( damage_type dt, body_part bp ) const override;

        float get_hit_base() const override;
        float get_dodge_base() const override;

        float  get_dodge() const override;       // Natural dodge, or 0 if we're occupied
        float  get_melee() const override; // For determining attack skill when awarding dodge practice.
        float  hit_roll() const override;  // For the purposes of comparing to player::dodge_roll()
        float  dodge_roll() override;  // For the purposes of comparing to player::hit_roll()

        int get_grab_strength() const; // intensity of grabbed effect

        monster_horde_attraction get_horde_attraction();
        void set_horde_attraction( monster_horde_attraction mha );
        bool will_join_horde( int size );

        /** Returns multiplier on fall damage at low velocity (knockback/pit/1 z-level, not 5 z-levels) */
        float fall_damage_mod() const override;
        /** Deals falling/collision damage with terrain/creature at pos */
        int impact( int force, const tripoint &pos ) override;

        bool has_grab_break_tec() const override;

        float stability_roll() const override;
        // We just dodged an attack from something
        void on_dodge( Creature *source, float difficulty ) override;
        // Something hit us (possibly null source)
        void on_hit( Creature *source, body_part bp_hit = num_bp,
                     float difficulty = INT_MIN, dealt_projectile_attack const *proj = nullptr ) override;
        // Get torso - monsters don't have body parts (yet?)
        body_part get_random_body_part( bool main ) const override;
        /** Returns vector containing all body parts this monster has. That is, { bp_torso } */
        std::vector<body_part> get_all_body_parts( bool only_main = false ) const override;

        /** Resets a given special to its monster type cooldown value */
        void reset_special( const std::string &special_name );
        /** Resets a given special to a value between 0 and its monster type cooldown value. */
        void reset_special_rng( const std::string &special_name );
        /** Sets a given special to the given value */
        void set_special( const std::string &special_name, int time );
        /** Sets the enabled flag for the given special to false */
        void disable_special( const std::string &special_name );

        void process_turn() override;
        /** Resets the value of all bonus fields to 0, clears special effect flags. */
        void reset_bonuses() override;
        /** Resets stats, and applies effects in an idempotent manner */
        void reset_stats() override;

        void die( Creature *killer ) override; //this is the die from Creature, it calls kill_mon
        void drop_items_on_death();

        // Other
        /**
         * Makes this monster into a fungus version
         * Returns false if no such monster exists
         */
        bool make_fungus();
        void make_friendly();
        /** Makes this monster an ally of the given monster. */
        void make_ally( const monster &z );
        // Add an item to inventory
        void add_item( const item &it );
        // check mech power levels and modify it.
        bool use_mech_power( int amt );
        bool check_mech_powered() const;
        int mech_str_addition() const;
        /**
         * Makes monster react to heard sound
         *
         * @param from Location of the sound source
         * @param source_volume Volume at the center of the sound source
         * @param distance Distance to sound source (currently just rl_dist)
         */
        void hear_sound( const tripoint &source, int vol, int distance );

        bool is_hallucination() const override;    // true if the monster isn't actually real

        field_type_id bloodType() const override;
        field_type_id gibType() const override;

        using Creature::add_msg_if_npc;
        void add_msg_if_npc( const std::string &msg ) const override;
        void add_msg_if_npc( game_message_type type, const std::string &msg ) const override;
        using Creature::add_msg_player_or_npc;
        void add_msg_player_or_npc( const std::string &player_msg,
                                    const std::string &npc_msg ) const override;
        void add_msg_player_or_npc( game_message_type type, const std::string &player_msg,
                                    const std::string &npc_msg ) const override;
        // TEMP VALUES
        tripoint wander_pos; // Wander destination - Just try to move in that direction
        int wandf;           // Urge to wander - Increased by sound, decrements each move
        std::vector<item> inv; // Inventory
        player *mounted_player = nullptr; // player that is mounting this creature
        character_id mounted_player_id; // id of player that is mounting this creature ( for save/load )
        character_id dragged_foe_id; // id of character being dragged by the monster
        cata::optional<item> tied_item; // item used to tie the monster
        cata::optional<item> battery_item; // item to power mechs
        // DEFINING VALUES
        int friendly;
        int anger = 0;
        int morale = 0;
        // Our faction (species, for most monsters)
        mfaction_id faction;
        // If we're related to a mission
        int mission_id;
        const mtype *type;
        // If true, don't spawn loot items as part of death.
        bool no_extra_death_drops;
        // If true, monster dies quietly and leaves no corpse.
        bool no_corpse_quiet = false;
        // Turned to false for simulating monsters during distant missions so they don't drop in sight.
        bool death_drops = true;
        bool is_dead() const;
        bool made_footstep;
        // If we're unique
        std::string unique_name;
        bool hallucination;
        // abstract for a fish monster representing a hidden stock of population in that area.
        int fish_population = 1;

        void setpos( const tripoint &p ) override;
        const tripoint &pos() const override;
        inline int posx() const override {
            return position.x;
        }
        inline int posy() const override {
            return position.y;
        }
        inline int posz() const override {
            return position.z;
        }

        short ignoring;
        cata::optional<time_point> lastseen_turn;

        // Stair data.
        int staircount;

        // Ammunition if we use a gun.
        std::map<std::string, int> ammo;

        /**
         * Convert this monster into an item (see @ref mtype::revert_to_itype).
         * Only useful for robots and the like, the monster must have at least
         * a non-empty item id as revert_to_itype.
         */
        item to_item() const;
        /**
         * Initialize values like speed / hp from data of an item.
         * This applies to robotic monsters that are spawned by invoking an item (e.g. turret),
         * and to reviving monsters that spawn from a corpse.
         */
        void init_from_item( const item &itm );

        time_point last_updated = calendar::turn_zero;
        /**
         * Do some cleanup and caching as monster is being unloaded from map.
         */
        void on_unload();
        /**
         * Retroactively update monster.
         */
        void on_load();


        const pathfinding_settings &get_pathfinding_settings() const override;
        std::set<tripoint> get_path_avoid() const override;
        // summoned monsters via spells
        void set_summon_time( const time_duration &length );
        // handles removing the monster if the timer runs out
        void decrement_summon_timer();
    private:
        void process_trigger( mon_trigger trig, int amount );
        void process_trigger( mon_trigger trig, const std::function<int()> &amount_func );

    private:
        int hp;
        std::map<std::string, mon_special_attack> special_attacks;
        tripoint goal;
        tripoint position;
        bool dead;
        /** Legacy loading logic for monsters that are packing ammo. **/
        void normalize_ammo( int old_ammo );
        /** Normal upgrades **/
        int next_upgrade_time();
        bool upgrades;
        int upgrade_time;
        bool reproduces;
        cata::optional<time_point> baby_timer;
        bool biosignatures;
        cata::optional<time_point> biosig_timer;
        monster_horde_attraction horde_attraction;
        /** Found path. Note: Not used by monsters that don't pathfind! **/
        std::vector<tripoint> path;
        std::bitset<NUM_MEFF> effect_cache;
        cata::optional<time_duration> summon_time_limit = cata::nullopt;

        player *find_dragged_foe();
        void nursebot_operate( player *dragged_foe );

    protected:
        void store( JsonOut &json ) const;
        void load( JsonObject &data );

        /** Processes monster-specific effects of an effect. */
        void process_one_effect( effect &it, bool is_new ) override;
};

#endif
