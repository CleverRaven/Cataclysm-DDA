#pragma once
#ifndef CATA_SRC_MONSTER_H
#define CATA_SRC_MONSTER_H

#include <bitset>
#include <climits>
#include <cstddef>
#include <functional>
#include <iosfwd>
#include <map>
#include <new>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include "calendar.h"
#include "character_id.h"
#include "color.h"
#include "compatibility.h"
#include "creature.h"
#include "damage.h"
#include "enums.h"
#include "point.h"
#include "type_id.h"
#include "units_fwd.h"
#include "value_ptr.h"
#include "weakpoint.h"

class Character;
class JsonObject;
class JsonOut;
class effect;
class effect_source;
class item;
struct monster_plan;
namespace catacurses
{
class window;
}  // namespace catacurses
struct dealt_projectile_attack;
struct pathfinding_settings;
struct trap;

enum class mon_trigger : int;

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
        explicit monster( const mtype_id &id );
        monster( const mtype_id &id, const tripoint &pos );
        monster( const monster & );
        monster( monster && ) noexcept( map_is_noexcept );
        ~monster() override;
        monster &operator=( const monster & );
        monster &operator=( monster && ) noexcept( string_is_noexcept );

        bool is_monster() const override {
            return true;
        }
        monster *as_monster() override {
            return this;
        }
        const monster *as_monster() const override {
            return this;
        }

        mfaction_id get_monster_faction() const override {
            return faction.id();
        }
        void poly( const mtype_id &id );
        bool can_upgrade() const;
        void hasten_upgrade();
        int get_upgrade_time() const;
        void allow_upgrade();
        void try_upgrade( bool pin_time );
        void try_reproduce();
        void try_biosignature();
        void refill_udders();
        void spawn( const tripoint &p );
        void spawn( const tripoint_abs_ms &loc );
        std::vector<material_id> get_absorb_material() const;
        creature_size get_size() const override;
        units::mass get_weight() const override;
        units::mass weight_capacity() const override;
        units::volume get_volume() const;
        int get_hp( const bodypart_id & ) const override;
        int get_hp() const override;
        int get_hp_max( const bodypart_id & ) const override;
        int get_hp_max() const override;
        int hp_percentage() const override;
        int get_eff_per() const override;

        float get_mountable_weight_ratio() const;

        static std::string speed_description( float mon_speed_rating,
                                              bool immobile = false,
                                              const speed_description_id &speed_desc = speed_description_id::NULL_ID() );

        // Access
        std::string get_name() const override;
        std::string name( unsigned int quantity = 1 ) const; // Returns the monster's formal name
        std::string name_with_armor() const; // Name, with whatever our armor is called
        // the creature-class versions of the above
        std::string disp_name( bool possessive = false, bool capitalize_first = false ) const override;
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
        bool can_see() const;      // MF_SEES and no MF_BLIND
        bool can_hear() const;     // MF_HEARS and no MF_DEAF
        bool can_submerge() const; // MF_AQUATIC or swims() or MF_NO_BREATH, and not MF_ELECTRONIC
        bool can_drown() const;    // MF_AQUATIC or swims() or MF_NO_BREATHE or flies()
        bool can_climb() const;         // climbs() or flies()
        bool digging() const override;  // digs() or can_dig() and diggable terrain
        bool can_dig() const;
        bool digs() const;
        bool flies() const;
        bool climbs() const;
        bool swims() const;
        // Returns false if the monster is stunned, has 0 moves or otherwise wouldn't act this turn
        bool can_act() const;
        int sight_range( float light_level ) const override;
        bool made_of( const material_id &m ) const override; // Returns true if it's made of m
        bool made_of_any( const std::set<material_id> &ms ) const override;
        bool made_of( phase_id p ) const; // Returns true if its phase is p

        bool shearable() const;

        bool avoid_trap( const tripoint &pos, const trap &tr ) const override;

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );
        void deserialize( const JsonObject &data, const tripoint_abs_sm &submap_loc );

        // Performs any necessary coordinate updates due to map shift.
        void shift( const point &sm_shift );
        void set_patrol_route( const std::vector<point> &patrol_pts_rel_ms );

        /**
         * Checks whether we can move to/through p. This does not account for bashing.
         *
         * This is used in pathfinding and ONLY checks the terrain. It ignores players
         * and monsters, which might only block this tile temporarily.
         * will_move_to() checks for impassable terrain etc
         * can_reach_to() checks for z-level difference.
         * can_move_to() is a wrapper for both of them.
         */
        bool can_move_to( const tripoint &p ) const;
        bool can_reach_to( const tripoint &p ) const;
        bool will_move_to( const tripoint &p ) const;

        bool will_reach( const point &p ); // Do we have plans to get to (x, y)?
        int  turns_to_reach( const point &p ); // How long will it take?

        // Returns true if the monster has a current goal
        bool has_dest() const;
        // Returns point at the end of the monster's current plans
        tripoint_abs_ms get_dest() const;
        // Returns the creature at the end of plans (if hostile)
        Creature *attack_target();
        // Go towards p using the monster's pathfinding settings.
        void set_dest( const tripoint_abs_ms &p );
        // Reset our plans, we've become aimless.
        void unset_dest();

        // Returns true if the monster has no plans.
        bool is_wandering() const;
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
        void wander_to( const tripoint_abs_ms &p, int f );

        // How good of a target is given creature (checks for visibility)
        float rate_target( Creature &c, float best, bool smart = false ) const;
        // is it mating season?
        bool mating_angry() const;
        void plan();
        void anger_hostile_seen( const monster_plan &mon_plan );
        void anger_mating_season( const monster_plan &mon_plan );
        // will change mon_plan::dist
        void anger_cub_threatened( monster_plan &mon_plan );
        void move(); // Actual movement
        void footsteps( const tripoint &p ); // noise made by movement
        void shove_vehicle( const tripoint &remote_destination,
                            const tripoint &nearby_destination ); // shove vehicles out of the way

        // check if the given square could drown a drownable monster
        bool is_aquatic_danger( const tripoint &at_pos ) const;

        // check if a monster at a position will drown and kill it if necessary
        // returns true if the monster dies
        // chance is the one_in( chance ) that the monster will drown
        bool die_if_drowning( const tripoint &at_pos, int chance = 1 );

        tripoint scent_move();
        int calc_movecost( const tripoint &f, const tripoint &t ) const;
        int calc_climb_cost( const tripoint &f, const tripoint &t ) const;

        bool is_immune_field( const field_type_id &fid ) const override;

        /**
         * Attempt to move to p.
         *
         * If there's something blocking the movement, such as infinite move
         * costs at the target, an existing NPC or monster, this function simply
         * aborts and does nothing.
         *
         * @param p Destination of movement
         * @param force If this is set to true, the movement will happen even if
         *              there's currently something, else than a creature, blocking the destination.
         * @param step_on_critter If this is set to true, the movement will happen even if
         *              there's currently a creature blocking the destination.
         *
         * @param stagger_adjustment is a multiplier for move cost to compensate for staggering.
         *
         * @return true if movement successful, false otherwise
         */
        bool move_to( const tripoint &p, bool force = false, bool step_on_critter = false,
                      float stagger_adjustment = 1.0 );

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
        int bash_skill() const;
        int bash_estimate() const;
        /** Returns ability of monster and any cooperative helpers to
         * bash the designated target.  **/
        int group_bash_skill( const tripoint &target );

        void stumble();
        void knock_back_to( const tripoint &to ) override;

        // Combat
        bool is_fleeing( Character &u ) const;
        monster_attitude attitude( const Character *u = nullptr ) const; // See the enum above
        Attitude attitude_to( const Creature &other ) const override;
        void process_triggers(); // Process things that anger/scare us

        bool is_underwater() const override;
        bool is_on_ground() const override;
        bool is_warm() const override;
        bool in_species( const species_id &spec ) const override;

        bool has_weapon() const override;
        bool is_dead_state() const override; // check if we should be dead or not
        bool is_elec_immune() const override;
        bool is_immune_effect( const efftype_id & ) const override;
        bool is_immune_damage( damage_type ) const override;

        void make_bleed( const effect_source &source, time_duration duration,
                         int intensity = 1, bool permanent = false, bool force = false, bool defferred = false );

        const weakpoint *absorb_hit( const weakpoint_attack &attack, const bodypart_id &bp,
                                     damage_instance &dam ) override;
        // The monster's skill in hitting a weakpoint
        float weakpoint_skill() const;

        bool block_hit( Creature *source, bodypart_id &bp_hit, damage_instance &d ) override;
        bool melee_attack( Creature &target );
        bool melee_attack( Creature &target, float accuracy );
        void melee_attack( Creature &p, bool ) = delete;
        void deal_projectile_attack( Creature *source, dealt_projectile_attack &attack,
                                     bool print_messages = true,
                                     const weakpoint_attack &wp_attack = weakpoint_attack() ) override;
        void deal_damage_handle_type( const effect_source &source, const damage_unit &du, bodypart_id bp,
                                      int &damage, int &pain ) override;
        void apply_damage( Creature *source, bodypart_id bp, int dam,
                           bool bypass_med = false ) override;
        // create gibs/meat chunks/blood etc all over the place, does not kill, can be called on a dead monster.
        void explode();
        // Let the monster die and let its body explode into gibs
        void die_in_explosion( Creature *source );

        void heal_bp( bodypart_id bp, int dam ) override;
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

        /** Returns a std::string containing effects for descriptions */
        std::string get_effect_status() const;

        float power_rating() const override;
        float speed_rating() const override;

        int get_worn_armor_val( damage_type dt ) const;
        int  get_armor_cut( bodypart_id bp ) const override; // Natural armor, plus any worn armor
        int  get_armor_bash( bodypart_id bp ) const override; // Natural armor, plus any worn armor
        int  get_armor_bullet( bodypart_id bp ) const override; // Natural armor, plus any worn armor
        int  get_armor_type( damage_type dt, bodypart_id bp ) const override;

        float get_hit_base() const override;
        float get_dodge_base() const override;

        float  get_dodge() const override;       // Natural dodge, or 0 if we're occupied
        float  get_melee() const override; // For determining attack skill when awarding dodge practice.
        float  hit_roll() const override;  // For the purposes of comparing to player::dodge_roll()
        float  dodge_roll() const override;  // For the purposes of comparing to player::hit_roll()

        bool can_attack_high() const override; // Can we attack upper limbs?
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
        void on_hit( Creature *source, bodypart_id bp_hit,
                     float difficulty = INT_MIN, dealt_projectile_attack const *proj = nullptr ) override;

        /** Resets a given special to its monster type cooldown value */
        void reset_special( const std::string &special_name );
        /** Resets a given special to a value between 0 and its monster type cooldown value. */
        void reset_special_rng( const std::string &special_name );
        /** Sets a given special to the given value */
        void set_special( const std::string &special_name, int time );
        /** Sets the enabled flag for the given special to false */
        void disable_special( const std::string &special_name );
        /** Test whether the monster has the specified special regardless of readiness. */
        bool has_special( const std::string &special_name ) const;
        /** Test whether the specified special is ready. */
        bool special_available( const std::string &special_name ) const;

        void process_turn() override;
        /** Resets the value of all bonus fields to 0, clears special effect flags. */
        void reset_bonuses() override;
        /** Resets stats, and applies effects in an idempotent manner */
        void reset_stats() override;

        void die( Creature *killer ) override; //this is the die from Creature, it calls kill_mo
        void drop_items_on_death( item *corpse );
        void spawn_dissectables_on_death( item *corpse ); //spawn dissectable CBMs into CORPSE pocket
        //spawn monster's inventory without killing it
        void generate_inventory( bool disableDrops = true );

        // Other
        /**
         * Makes this monster into a fungus version
         * Returns false if no such monster exists
         * Returns true if monster is immune or if it got fungalized
         */
        bool make_fungus();
        void make_friendly();
        /** Makes this monster an ally of the given monster. */
        void make_ally( const monster &z );
        // Add an item to inventory
        void add_item( const item &it );

        /**
        * Consume UPS from mech battery.
        * @param amt amount of energy to consume. Is rounded down to kJ precision. Do not use negative values.
        * @return Actual amount of energy consumed
        */
        units::energy use_mech_power( units::energy amt );
        bool check_mech_powered() const;
        int mech_str_addition() const;

        /**
         * Makes monster react to heard sound
         *
         * @param source Location of the sound source
         * @param vol Volume at the center of the sound source
         * @param distance Distance to sound source (currently just rl_dist)
         */
        void hear_sound( const tripoint &source, int vol, int distance, bool provocative );

        bool is_hallucination() const override;    // true if the monster isn't actually real

        bool is_electrical() const override;    // true if the monster produces electric radiation

        field_type_id bloodType() const override;
        field_type_id gibType() const override;

        using Creature::add_msg_if_npc;
        void add_msg_if_npc( const std::string &msg ) const override;
        void add_msg_if_npc( const game_message_params &params, const std::string &msg ) const override;
        using Creature::add_msg_debug_if_npc;
        void add_msg_debug_if_npc( debugmode::debug_filter type, const std::string &msg ) const override;
        using Creature::add_msg_player_or_npc;
        void add_msg_player_or_npc( const std::string &player_msg,
                                    const std::string &npc_msg ) const override;
        void add_msg_player_or_npc( const game_message_params &params, const std::string &player_msg,
                                    const std::string &npc_msg ) const override;
        using Creature::add_msg_debug_player_or_npc;
        void add_msg_debug_player_or_npc( debugmode::debug_filter type, const std::string &player_msg,
                                          const std::string &npc_msg ) const override;

        tripoint_abs_ms wander_pos; // Wander destination - Just try to move in that direction
        bool provocative_sound = false; // Are we wandering toward something we think is alive?
        int wandf = 0;       // Urge to is_wandering - Increased by sound, decrements each move
        std::vector<item> inv; // Inventory
        std::vector<item> dissectable_inv; // spawned at death, tracked for respawn/dissection
        Character *mounted_player = nullptr; // player that is mounting this creature
        character_id mounted_player_id; // id of player that is mounting this creature ( for save/load )
        character_id dragged_foe_id; // id of character being dragged by the monster
        cata::value_ptr<item> tied_item; // item used to tie the monster
        cata::value_ptr<item> tack_item; // item representing saddle and reins and such
        cata::value_ptr<item> armor_item; // item of armor the monster may be wearing
        cata::value_ptr<item> storage_item; // storage item for monster carrying items
        cata::value_ptr<item> battery_item; // item to power mechs
        units::mass get_carried_weight() const;
        units::volume get_carried_volume() const;
        void move_special_item_to_inv( cata::value_ptr<item> &it );

        // DEFINING VALUES
        int friendly = 0;
        int anger = 0;
        int morale = 0;
        // Our faction (species, for most monsters)
        mfaction_id faction;
        // If we're related to a mission
        std::set<int> mission_ids;
        // Names of mission monsters fused with this monster
        std::vector<std::string> mission_fused;
        const mtype *type;
        // If true, don't spawn loot items as part of death.
        bool no_extra_death_drops = false;
        // If true, monster dies quietly and leaves no corpse.
        bool no_corpse_quiet = false;
        // Turned to false for simulating monsters during distant missions so they don't drop in sight.
        bool death_drops = true;
        // If true, sound and message is suppressed for monster death.
        bool quiet_death = false;
        bool is_dead() const;
        bool made_footstep = false;
        //if we are a nemesis monster from the 'hunted' trait
        bool is_nemesis() const;
        // If we're unique
        std::string unique_name;
        // Player given nickname
        std::string nickname;
        bool hallucination = false;
        // abstract for a fish monster representing a hidden stock of population in that area.
        int fish_population = 1;

        short ignoring = 0;
        bool aggro_character = true;

        std::optional<time_point> lastseen_turn;

        // Stair data.
        int staircount = 0;

        // Ammunition if we use a gun.
        std::map<itype_id, int> ammo;

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
        void init_from_item( item &itm );

        /**
         * Do some cleanup and caching as monster is being unloaded from map.
         */
        void on_unload();
        /**
         * Retroactively update monster.
         * Call this after a preexisting monster has been placed on map.
         * Don't call for monsters that have been freshly created, it may cause
         * the monster to upgrade itself into another monster type.
         */
        void on_load();

        const pathfinding_settings &get_pathfinding_settings() const override;
        std::set<tripoint> get_path_avoid() const override;
    private:
        void process_trigger( mon_trigger trig, int amount );
        void process_trigger( mon_trigger trig, const std::function<int()> &amount_func );

        int hp = 0;
        std::map<std::string, mon_special_attack> special_attacks;
        std::optional<tripoint_abs_ms> goal;
        bool dead = false;
        /** Normal upgrades **/
        int next_upgrade_time();
        bool upgrades = false;
        int upgrade_time = 0;
        bool reproduces = false;
        std::optional<time_point> baby_timer;
        bool biosignatures = false;
        std::optional<time_point> biosig_timer;
        time_point udder_timer;
        monster_horde_attraction horde_attraction = MHA_NULL;
        /** Found path. Note: Not used by monsters that don't pathfind! **/
        std::vector<tripoint> path;
        /** patrol points for monsters that can pathfind and have a patrol route! **/
        std::vector<tripoint_abs_ms> patrol_route;
        int next_patrol_point = -1;

        std::bitset<NUM_MEFF> effect_cache;
        int turns_since_target = 0;

        Character *find_dragged_foe();
        void nursebot_operate( Character *dragged_foe );

    protected:
        void store( JsonOut &json ) const;
        void load( const JsonObject &data );
        void load( const JsonObject &data, const tripoint_abs_sm &submap_loc );

        void on_move( const tripoint_abs_ms &old_pos ) override;
        /** Processes monster-specific effects of an effect. */
        void process_one_effect( effect &it, bool is_new ) override;
};

#endif // CATA_SRC_MONSTER_H
