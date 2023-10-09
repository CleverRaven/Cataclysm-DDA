#pragma once
#ifndef CATA_SRC_CREATURE_H
#define CATA_SRC_CREATURE_H

#include <array>
#include <climits>
#include <iosfwd>
#include <map>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <queue>

#include "bodypart.h"
#include "compatibility.h"
#include "damage.h"
#include "debug.h"
#include "effect_source.h"
#include "enums.h"
#include "field_type.h"
#include "pimpl.h"
#include "string_formatter.h"
#include "talker.h"
#include "type_id.h"
#include "units_fwd.h"
#include "viewer.h"
#include "weakpoint.h"

class monster;
class translation;
template <typename T> struct enum_traits;

enum game_message_type : int;
class effect;
class effects_map;
class nc_color;

namespace catacurses
{
class window;
} // namespace catacurses
class body_part_set;
class Character;
class JsonObject;
class JsonOut;
class anatomy;
class avatar;
class field;
class field_entry;
class npc;
class time_duration;
struct point;
struct tripoint;

struct mon_flag;
struct dealt_projectile_attack;
struct pathfinding_settings;
struct projectile;
struct projectile_attack_results;
struct trap;

using anatomy_id = string_id<anatomy>;

enum class creature_size : int {
    // Keep it starting at 1 - damage done to monsters depends on it
    // Squirrel
    tiny = 1,
    // Dog
    small,
    // Human
    medium,
    // Cow
    large,
    // TAAAANK
    huge,
    // must always be at the end, is actually number + 1 since we start counting at 1
    num_sizes
};

using I = std::underlying_type_t<creature_size>;
constexpr I operator+( const creature_size lhs, const creature_size rhs )
{
    return static_cast<I>( lhs ) + static_cast<I>( rhs );
}

constexpr I operator+( const creature_size lhs, const I rhs )
{
    return static_cast<I>( lhs ) + rhs;
}

constexpr I operator+( const I lhs, const creature_size rhs )
{
    return lhs + static_cast<I>( rhs );
}

constexpr I operator-( const creature_size lhs, const creature_size rhs )
{
    return static_cast<I>( lhs ) - static_cast<I>( rhs );
}

constexpr I operator-( const creature_size lhs, const I rhs )
{
    return static_cast<I>( lhs ) - rhs;
}

constexpr I operator-( const I lhs, const creature_size rhs )
{
    return lhs - static_cast<I>( rhs );
}

constexpr I operator*( const creature_size lhs, const creature_size rhs )
{
    return static_cast<I>( lhs ) * static_cast<I>( rhs );
}

constexpr I operator*( const creature_size lhs, const I rhs )
{
    return static_cast<I>( lhs ) * rhs;
}

constexpr I operator*( const I lhs, const creature_size rhs )
{
    return lhs * static_cast<I>( rhs );
}

constexpr I operator/( const creature_size lhs, const creature_size rhs )
{
    return static_cast<I>( lhs ) / static_cast<I>( rhs );
}

constexpr I operator/( const creature_size lhs, const I rhs )
{
    return static_cast<I>( lhs ) / rhs;
}

constexpr bool operator<=( const creature_size lhs, const creature_size rhs )
{
    return static_cast<I>( lhs ) <= static_cast<I>( rhs );
}

constexpr bool operator<=( const creature_size lhs, const I rhs )
{
    return static_cast<I>( lhs ) <= rhs;
}

constexpr bool operator<=( const I lhs, const creature_size rhs )
{
    return lhs <= static_cast<I>( rhs );
}

constexpr bool operator<( const creature_size lhs, const creature_size rhs )
{
    return static_cast<I>( lhs ) < static_cast<I>( rhs );
}

constexpr bool operator<( const creature_size lhs, const I rhs )
{
    return static_cast<I>( lhs ) < rhs;
}

constexpr bool operator<( const I lhs, const creature_size rhs )
{
    return lhs < static_cast<I>( rhs );
}

constexpr bool operator>=( const creature_size lhs, const creature_size rhs )
{
    return static_cast<I>( lhs ) >= static_cast<I>( rhs );
}

constexpr bool operator>=( const creature_size lhs, const I rhs )
{
    return static_cast<I>( lhs ) >= rhs;
}

constexpr bool operator>=( const I lhs, const creature_size rhs )
{
    return lhs >= static_cast<I>( rhs );
}

constexpr bool operator>( const creature_size lhs, const creature_size rhs )
{
    return static_cast<I>( lhs ) > static_cast<I>( rhs );
}

constexpr bool operator>( const creature_size lhs, const I rhs )
{
    return static_cast<I>( lhs ) > rhs;
}

constexpr bool operator>( const I lhs, const creature_size rhs )
{
    return lhs > static_cast<I>( rhs );
}

constexpr bool operator==( const creature_size lhs, const I rhs )
{
    return static_cast<I>( lhs ) == rhs;
}

constexpr bool operator==( const I lhs, const creature_size rhs )
{
    return lhs == static_cast<I>( rhs );
}

enum class FacingDirection : int {
    NONE = 0,
    LEFT = 1,
    RIGHT = 2
};

enum class get_body_part_flags : int {
    none = 0,
    only_main = 1 << 0,
    sorted = 1 << 1,
    primary_type = 1 << 2,
    only_minor = 1 << 3
};

template<>
struct enum_traits<get_body_part_flags> {
    static constexpr bool is_flag_enum = true;
};

enum class body_part_filter : int {
    strict = 0,
    equivalent = 1,
    next_best = 2
};

using scheduled_effect = struct scheduled_effect_t {
    efftype_id eff_id;
    time_duration dur;
    bodypart_id bp;
    bool permanent = false;
    int intensity = 0;
    bool force = false;
    bool deferred = false;
};

using terminating_effect = struct terminating_effect_t {
    efftype_id eff_id;
    bodypart_id bp;
};

class Creature : public viewer
{
    public:
        ~Creature() override;

        static const std::map<std::string, creature_size> size_map;

        // Like disp_name, but without any "the"
        virtual std::string get_name() const = 0;
        virtual std::string disp_name( bool possessive = false,
                                       bool capitalize_first = false ) const = 0; // displayname for Creature
        virtual std::string skin_name() const = 0; // name of outer layer, e.g. "armor plates"

        virtual std::vector<std::string> get_grammatical_genders() const;

        virtual bool is_avatar() const {
            return false;
        }
        virtual bool is_npc() const {
            return false;
        }
        virtual bool is_monster() const {
            return false;
        }
        virtual Character *as_character() {
            return nullptr;
        }
        virtual const Character *as_character() const {
            return nullptr;
        }
        virtual avatar *as_avatar() {
            return nullptr;
        }
        virtual const avatar *as_avatar() const {
            return nullptr;
        }
        virtual npc *as_npc() {
            return nullptr;
        }
        virtual const npc *as_npc() const {
            return nullptr;
        }
        virtual monster *as_monster() {
            return nullptr;
        }
        virtual const monster *as_monster() const {
            return nullptr;
        }
        virtual mfaction_id get_monster_faction() const = 0;

        // Witness and respond to theft of faction items
        virtual void witness_thievery( item *it ) = 0;
        /** return the direction the creature is facing, for sdl horizontal flip **/
        FacingDirection facing = FacingDirection::RIGHT;
        /** Returns true for non-real Creatures used temporarily; i.e. fake NPC's used for turret fire. */
        virtual bool is_fake() const;
        /** Sets a Creature's fake boolean. */
        virtual void set_fake( bool fake_value );
        // TODO: fix point types (remove pos() and rename pos_bub() to be the new pos())
        tripoint pos() const;
        tripoint_bub_ms pos_bub() const;
        inline int posx() const {
            return pos().x;
        }
        inline int posy() const {
            return pos().y;
        }
        inline int posz() const {
            return get_location().z();
        }
        void setpos( const tripoint &p );
        /** Moves the creature to the given location and calls the on_move() handler. */
        void move_to( const tripoint_abs_ms &loc );

        /** Recreates the Creature from scratch. */
        virtual void normalize();
        /** Processes effects and bonuses and allocates move points based on speed. */
        virtual void process_turn();
        /** Resets the value of all bonus fields to 0. */
        virtual void reset_bonuses();
        /** Resets stats, and applies effects in an idempotent manner */
        virtual void reset_stats() = 0;
        /** Handles stat and bonus reset. */
        virtual void reset();
        /** Adds an appropriate blood splatter. */
        virtual void bleed() const;
        /** Empty function. Should always be overwritten by the appropriate player/NPC/monster version. */
        virtual void die( Creature *killer ) = 0;

        /** Should always be overwritten by the appropriate player/NPC/monster version. */
        virtual float hit_roll() const = 0;
        virtual float dodge_roll() const = 0;
        virtual float stability_roll() const = 0;
        virtual bool can_attack_high() const = 0;

        /**
         * Simplified attitude towards any creature:
         * hostile - hate, want to kill, etc.
         * neutral - anything between.
         * friendly - avoid harming it, maybe even help.
         * any - any of the above, used in safemode_ui
         */
        enum class Attitude : int {
            HOSTILE,
            NEUTRAL,
            FRIENDLY,
            ANY
        };

        /**
         * Simplified attitude string for unlocalized needs.
         */
        static std::string attitude_raw_string( Attitude att );

        /**
         * Creature Attitude as String and color
         */
        static const std::pair<translation, nc_color> &get_attitude_ui_data( Attitude att );

        /**
         * Attitude (of this creature) towards another creature. This might not be symmetric.
         */
        virtual Attitude attitude_to( const Creature &other ) const = 0;

        /**
         * Called when a creature triggers a trap, returns true if they don't set it off.
         * @param tr is the trap that was triggered.
         * @param pos is the location of the trap (not necessarily of the creature) in the main map.
         */
        virtual bool avoid_trap( const tripoint &pos, const trap &tr ) const = 0;

        /**
         * The functions check whether this creature can see the target.
         * The target may either be another creature (critter), or a specific point on the map.
         *
         * The function that take another creature as input should check visibility of that creature
         * (e.g. not digging, or otherwise invisible). They must than check whether the location of
         * the other monster is visible.
         */
        /*@{*/
        bool sees( const Creature &critter ) const override;
        bool sees( const tripoint &t, bool is_avatar = false, int range_mod = 0 ) const override;
        bool sees( const tripoint_bub_ms &t, bool is_avatar = false, int range_mod = 0 ) const override;
        /*@}*/

        /**
         * How far the creature sees under the given light. Places outside this range can
         * @param light_level See @ref game::light_level.
         */
        virtual int sight_range( float light_level ) const = 0;

        /** Returns an approximation of the creature's strength. */
        virtual float power_rating() const = 0;
        /** Returns an approximate number of tiles this creature can travel per turn. */
        virtual float speed_rating() const = 0;
        /**
         * For fake-players (turrets, mounted turrets) this functions
         * chooses a target. This is for creatures that are friendly towards
         * the player and therefore choose a target that is hostile
         * to the player.
         *
         * @param range The maximal range to look for monsters, anything
         * outside of that range is ignored.
         * @param boo_hoo The number of targets that have been skipped
         * because the player is in the way.
         * @param area The area of effect of the projectile aimed.
         */
        Creature *auto_find_hostile_target( int range, int &boo_hoo, int area = 0 );

        /**
         * Size of the target this creature presents to ranged weapons.
         * 0.0 means unhittable, 1.0 means all projectiles going through this creature's tile will hit it.
         */
        double ranged_target_size() const;

        // handles blocking of damage instance. mutates &dam
        virtual bool block_hit( Creature *source, bodypart_id &bp_hit,
                                damage_instance &dam ) = 0;

        // handles armor absorption (including clothing damage etc)
        // of damage instance. returns name of weakpoint hit, if any. mutates &dam.
        virtual const weakpoint *absorb_hit( const weakpoint_attack &attack, const bodypart_id &bp,
                                             damage_instance &dam ) = 0;

        // TODO: this is just a shim so knockbacks work
        void knock_back_from( const tripoint &p );
        virtual void knock_back_to( const tripoint &to ) = 0;

        // Converts the "cover_vitals" protection on the specified body part into
        // a modifier (between 0 and 1) that would be applied to incoming critical damage
        float get_crit_factor( const bodypart_id &bp ) const;

        int size_melee_penalty() const;
        // begins a melee attack against the creature
        // returns hit - dodge (>=0 = hit, <0 = miss)
        virtual int deal_melee_attack( Creature *source, int hitroll );

        // Distance == 1 and on the same z-level or with a clear shot up/down.
        // If allow_zlev is false, don't allow attacking up/down at all.
        // If allow_zlev is true, also allow distance == 1 and on different z-level
        // as long as floor/ceiling doesn't exist.
        // Also check other factors, like vehicle separating deep water/air
        bool is_adjacent( const Creature *target, bool allow_z_levels ) const;

        // modifies the damage dealt based on the creature's enchantments
        // since creatures currently don't have enchantments, this is just virtual
        virtual damage_instance modify_damage_dealt_with_enchantments( const damage_instance &dam ) const;
        // completes a melee attack against the creature
        // dealt_dam is overwritten with the values of the damage dealt
        virtual void deal_melee_hit( Creature *source, int hit_spread, bool critical_hit,
                                     damage_instance dam, dealt_damage_instance &dealt_dam,
                                     const weakpoint_attack &attack = weakpoint_attack(),
                                     const bodypart_id *bp = nullptr );

        // Makes a ranged projectile attack against the creature
        // Sets relevant values in `attack`.
        virtual void deal_projectile_attack( Creature *source, dealt_projectile_attack &attack,
                                             bool print_messages = true, const weakpoint_attack &wp_attack = weakpoint_attack() );

        /**
         * Deals the damage via an attack. Allows armor mitigation etc.
         *
         * Most sources of external damage should use deal_damage
         * Mutates the damage_instance& object passed in to reflect the
         * post-mitigation object.
         * Does nothing if this creature is already dead.
         * Does not call @ref check_dead_state.
         * @param source The attacking creature, can be null.
         * @param bp The attacked body part
         * @param dam The damage dealt
         */
        virtual dealt_damage_instance deal_damage( Creature *source, bodypart_id bp,
                const damage_instance &dam, const weakpoint_attack &attack = weakpoint_attack() );
        // for each damage type, how much gets through and how much pain do we
        // accrue? mutates damage and pain
        virtual void deal_damage_handle_type( const effect_source &source, const damage_unit &du,
                                              bodypart_id bp, int &damage, int &pain );

        // directly decrements the damage. ONLY handles damage, doesn't
        // increase pain, apply effects, etc
        virtual void apply_damage( Creature *source, bodypart_id bp, int amount,
                                   bool bypass_med = false ) = 0;

        virtual void heal_bp( bodypart_id bp, int dam );

        /**
         * Attempts to pull the target at point p towards this creature.
         * @param name Name of the implement used to pull the target.
         * @param p Position of the target creature.
        */
        void longpull( const std::string &name, const tripoint &p );

        bool dodge_check( float hit_roll, bool force_try = false );
        bool dodge_check( monster *z );
        bool dodge_check( monster *z, bodypart_id bp, const damage_instance &dam_inst );

        // Temporarily reveals an invisible player when a monster tries to enter their location
        bool stumble_invis( const Creature &player, bool stumblemsg = true );
        // Attack an empty location
        bool attack_air( const tripoint &p );

        /**
         * This creature just dodged an attack - possibly special/ranged attack - from source.
         * Players should train dodge, monsters may use some special defenses.
         */
        virtual void on_dodge( Creature *source, float difficulty ) = 0;
        /**
         * Invoked when the creature attempts to dodge, regardless of success or failure.
         */
        virtual void on_try_dodge() = 0;
        /**
         * This creature just got hit by an attack - possibly special/ranged attack - from source.
         * Players should train dodge, possibly counter-attack somehow.
         */
        virtual void on_hit( Creature *source, bodypart_id bp_hit,
                             float difficulty = INT_MIN, dealt_projectile_attack const *proj = nullptr ) = 0;

        /** Returns true if this monster has a gun-type attack or the RANGED_ATTACKER flag*/
        virtual bool is_ranged_attacker() const;

        virtual bool digging() const;
        virtual bool is_on_ground() const = 0;
        bool cant_do_underwater( bool msg = true ) const;
        virtual bool is_underwater() const;
        bool is_likely_underwater() const; // Should eventually be virtual, although not pure
        virtual bool is_warm() const; // is this creature warm, for IR vision, heat drain, etc
        virtual bool in_species( const species_id & ) const;

        virtual bool has_weapon() const = 0;
        virtual bool is_hallucination() const = 0;

        // returns true if the creature has an electric field
        virtual bool is_electrical() const = 0;

        // returns true if the creature has an electric field
        virtual bool is_nether() const = 0;

        // returns true if health is zero or otherwise should be dead
        virtual bool is_dead_state() const = 0;

        // Resistances
        virtual bool is_elec_immune() const = 0;
        virtual bool is_immune_effect( const efftype_id &type ) const = 0;
        virtual bool is_immune_damage( const damage_type_id &type ) const = 0;

        // Field dangers
        /** Returns true if there is a field in the field set that is dangerous to us. */
        bool is_dangerous_fields( const field &fld ) const;
        /** Returns true if the given field entry is dangerous to us. */
        bool is_dangerous_field( const field_entry &entry ) const;
        /** Returns true if we are immune to the field type with the given fid. Can handle intensity if field intensity data is specified for the effect
        , so this function should only be called through is_dangerous_field().
         */
        virtual bool is_immune_field( const field_type_id & ) const {
            return false;
        }

        // check if the creature is immune to the effect / field based on the immunity data
        virtual bool check_immunity_data( const field_immunity_data & ) const {
            return false;
        }

        /** Returns multiplier on fall damage at low velocity (knockback/pit/1 z-level, not 5 z-levels) */
        virtual float fall_damage_mod() const = 0;
        /** Deals falling/collision damage with terrain/creature at pos */
        virtual int impact( int force, const tripoint &pos ) = 0;

        /**
         * This function checks the creatures @ref is_dead_state and (if true) calls @ref die.
         * You can either call this function after hitting this creature, or let the game
         * call it during @ref game::cleanup_dead.
         * As @ref die has many side effects (messages, on-death-triggers, ...), you should be
         * careful when calling this and expect that at least a "The monster dies!" message might
         * have been printed. If you want to print any message relating to the attack (e.g. how
         * much damage has been dealt, how the attack was performed, what has been blocked...), do
         * it *before* calling this function.
         */
        void check_dead_state();

        /** Processes move stopping effects. Returns false if movement is stopped. */
        virtual bool move_effects( bool attacking ) = 0;

        // Next three functions don't do anything but forward to next functions with nullptr
        // as source they should be removed once all effect sources are assigned
        void add_effect( const effect &eff, bool force = false, bool deferred = false ) {
            add_effect( effect_source::empty(), eff, force, deferred );
        }
        void add_effect( const efftype_id &eff_id, const time_duration &dur, bodypart_id bp,
                         bool permanent = false, int intensity = 0, bool force = false, bool deferred = false ) {
            add_effect( effect_source::empty(), eff_id, dur, bp, permanent, intensity, force, deferred );
        }
        void add_effect( const efftype_id &eff_id, const time_duration &dur, bool permanent = false,
                         int intensity = 0, bool force = false, bool deferred = false ) {
            add_effect( effect_source::empty(), eff_id, dur, permanent, intensity, force, deferred );
        }

        void add_effect( const effect_source &source, const effect &eff, bool force = false,
                         bool deferred = false );
        /** Adds or modifies an effect. If intensity is given it will set the effect intensity
            to the given value, or as close as max_intensity values permit. */
        void add_effect( const effect_source &source, const efftype_id &eff_id, const time_duration &dur,
                         bodypart_id bp, bool permanent = false, int intensity = 0, bool force = false,
                         bool deferred = false );
        void add_effect( const effect_source &source, const efftype_id &eff_id, const time_duration &dur,
                         bool permanent = false, int intensity = 0, bool force = false, bool deferred = false );
        /** Schedules a new effect to be applied. Used during effect processing to avoid invalidating
            current effects map. */
        void schedule_effect( const effect &eff, bool force = false,
                              bool deferred = false );
        void schedule_effect( const efftype_id &eff_id, const time_duration &dur,
                              bodypart_id bp, bool permanent = false, int intensity = 0, bool force = false,
                              bool deferred = false );
        void schedule_effect( const efftype_id &eff_id, const time_duration &dur,
                              bool permanent = false, int intensity = 0, bool force = false, bool deferred = false );
        /** Gives chance to save via environmental resist, returns false if resistance was successful. */
        bool add_env_effect( const efftype_id &eff_id, const bodypart_id &vector, int strength,
                             const time_duration &dur, const bodypart_id &bp, bool permanent = false, int intensity = 1,
                             bool force = false );
        bool add_env_effect( const efftype_id &eff_id, const bodypart_id &vector, int strength,
                             const time_duration &dur, bool permanent = false, int intensity = 1, bool force = false );
        /** Removes a listed effect. If the bodypart is not specified remove all effects of
         * a given type, targeted or untargeted. Returns true if anything was
         * removed. */
        bool remove_effect( const efftype_id &eff_id, const bodypart_id &bp );
        bool remove_effect( const efftype_id &eff_id );
        /** Schedule effect removal */
        void schedule_effect_removal( const efftype_id &eff_id, const bodypart_id &bp );
        void schedule_effect_removal( const efftype_id &eff_id );
        /** Remove all effects. */
        void clear_effects();
        /** Check if creature has the matching effect. If the bodypart is not specified check if the Creature has any effect
         *  of the matching type, targeted or untargeted. */
        bool has_effect( const efftype_id &eff_id, const bodypart_id &bp ) const;
        bool has_effect( const efftype_id &eff_id ) const;
        /** Check if creature has any effect with the given flag. */
        bool has_effect_with_flag( const flag_id &flag, const bodypart_id &bp ) const;
        bool has_effect_with_flag( const flag_id &flag ) const;
        std::vector<std::reference_wrapper<const effect>> get_effects_with_flag(
                    const flag_id &flag ) const;
        std::vector<std::reference_wrapper<const effect>> get_effects_from_bp(
                    const bodypart_id &bp ) const;
        std::vector<std::reference_wrapper<const effect>> get_effects() const;

        /** Return the effect that matches the given arguments exactly. */
        const effect &get_effect( const efftype_id &eff_id,
                                  const bodypart_id &bp = bodypart_str_id::NULL_ID() ) const;
        effect &get_effect( const efftype_id &eff_id, const bodypart_id &bp = bodypart_str_id::NULL_ID() );
        /** Returns the duration of the matching effect. Returns 0 if effect doesn't exist. */
        time_duration get_effect_dur( const efftype_id &eff_id,
                                      const bodypart_id &bp = bodypart_str_id::NULL_ID() ) const;
        /** Returns the intensity of the matching effect. Returns 0 if effect doesn't exist. */
        int get_effect_int( const efftype_id &eff_id,
                            const bodypart_id &bp = bodypart_str_id::NULL_ID() ) const;
        /** Returns true if the creature resists an effect */
        bool resists_effect( const effect &e ) const;

        // Methods for setting/getting misc key/value pairs.
        void set_value( const std::string &key, const std::string &value );
        void remove_value( const std::string &key );
        std::string get_value( const std::string &key ) const;
        void clear_values();

        virtual units::mass get_weight() const = 0;

        /** Processes through all the effects on the Creature. */
        virtual void process_effects();

        /** Returns true if the player has the entered trait, returns false for non-humans */
        virtual bool has_trait( const trait_id &flag ) const;

        // not-quite-stats, maybe group these with stats later
        virtual void mod_pain( int npain );
        virtual void mod_pain_noresist( int npain );
        virtual void set_pain( int npain );
        virtual int get_pain() const;
        virtual int get_perceived_pain() const;

        int get_moves() const;
        void mod_moves( int nmoves );
        void set_moves( int nmoves );

        virtual bool in_sleep_state() const;

        /*
         * Get/set our killer, this is currently used exclusively to allow
         * mondeath effects to happen after death cleanup
         */
        virtual Creature *get_killer() const;

        /*
         * Getters for stats - combat-related stats will all be held within
         * the Creature and re-calculated during every normalize() call
         */
        virtual int get_num_blocks() const;
        virtual int get_num_dodges() const;
        virtual int get_num_blocks_bonus() const;
        virtual int get_num_dodges_bonus() const;
        virtual int get_num_dodges_base() const;
        virtual int get_num_blocks_base() const;

        virtual int get_env_resist( bodypart_id bp ) const;

        virtual int get_armor_res( const damage_type_id &dt, bodypart_id bp ) const;
        virtual int get_armor_res_base( const damage_type_id &dt, bodypart_id bp ) const;
        virtual int get_armor_res_bonus( const damage_type_id &dt ) const;
        virtual int get_spell_resist() const;

        virtual int get_armor_type( const damage_type_id &dt, bodypart_id bp ) const = 0;

        virtual float get_dodge() const;
        virtual float get_melee() const = 0;
        virtual float get_hit() const;

        virtual int get_speed() const;
        virtual int get_eff_per() const;
        virtual creature_size get_size() const = 0;
        virtual int get_hp( const bodypart_id &bp ) const;
        virtual int get_hp() const;
        virtual int get_hp_max( const bodypart_id &bp ) const;
        virtual int get_hp_max() const;
        virtual int hp_percentage() const = 0;
        virtual bool made_of( const material_id &m ) const = 0;
        virtual bool made_of_any( const std::set<material_id> &ms ) const = 0;
        // standard creature material sets
        static const std::set<material_id> cmat_flesh;
        static const std::set<material_id> cmat_fleshnveg;
        static const std::set<material_id> cmat_flammable;
        static const std::set<material_id> cmat_flameres;
        virtual field_type_id bloodType() const = 0;
        virtual field_type_id gibType() const = 0;
        // TODO: replumb this to use a std::string along with monster flags.
        virtual bool has_flag( const mon_flag_id & ) const {
            return false;
        }
        virtual bool uncanny_dodge() {
            return false;
        }

    private:
        /** The creature's position in absolute coordinates */
        tripoint_abs_ms location;
    protected:
        // Sets the creature's position without any side-effects.
        void set_pos_only( const tripoint &p );
        // Sets the creature's position without any side-effects.
        void set_location( const tripoint_abs_ms &loc );
        // Invoked when the creature's position changes.
        virtual void on_move( const tripoint_abs_ms &old_pos );
        /**anatomy is the plan of the creature's body*/
        anatomy_id creature_anatomy = anatomy_id( "default_anatomy" );
        /**this is the actual body of the creature*/
        std::map<bodypart_str_id, bodypart> body;
    public:
        anatomy_id get_anatomy() const;
        void set_anatomy( const anatomy_id &anat );

        bodypart_id get_random_body_part( bool main = false ) const;
        /**
         * Returns body parts this creature have.
         * @param only_main If true, only displays parts that can have hit points
         */
        std::vector<bodypart_id> get_all_body_parts(
            get_body_part_flags = get_body_part_flags::none ) const;
        std::vector<bodypart_id> get_all_body_parts_of_type(
            body_part_type::type part_type,
            get_body_part_flags flags = get_body_part_flags::none ) const;
        bodypart_id get_random_body_part_of_type( body_part_type::type part_type ) const;
        bodypart_id get_root_body_part() const;
        /* Returns all body parts with the given flag */
        std::vector<bodypart_id> get_all_body_parts_with_flag( const json_character_flag &flag ) const;
        /* Returns the bodyparts to drench : upper/mid/lower correspond to the appropriate limb flag */
        body_part_set get_drenching_body_parts( bool upper = true, bool mid = true,
                                                bool lower = true ) const;

        /* Returns the number of bodyparts of a given type*/
        int get_num_body_parts_of_type( body_part_type::type part_type ) const;

        /* Returns the number of broken bodyparts of a given type */
        int get_num_broken_body_parts_of_type( body_part_type::type part_type ) const;

        const std::map<bodypart_str_id, bodypart> &get_body() const;
        void set_body();
        // Does not fire debug message if part does not exist
        bool has_part( const bodypart_id &id, body_part_filter filter = body_part_filter::strict ) const;
        // A debug message will be fired if part does not exist.
        // Check with has_part first if a part may not exist.
        bodypart *get_part( const bodypart_id &id );
        const bodypart *get_part( const bodypart_id &id ) const;

        // get the body part id that matches for the character
        bodypart_id get_part_id( const bodypart_id &id,
                                 body_part_filter filter = body_part_filter::next_best, bool suppress_debugmsg = false ) const;

        int get_part_hp_cur( const bodypart_id &id ) const;
        int get_part_hp_max( const bodypart_id &id ) const;

        int get_part_healed_total( const bodypart_id &id ) const;
        int get_part_damage_disinfected( const bodypart_id &id ) const;
        int get_part_damage_bandaged( const bodypart_id &id ) const;
        int get_part_drench_capacity( const bodypart_id &id ) const;
        int get_part_wetness( const bodypart_id &id ) const;
        int get_part_temp_cur( const bodypart_id &id ) const;
        int get_part_temp_conv( const bodypart_id &id ) const;
        int get_part_frostbite_timer( const bodypart_id &id )const;

        std::array<int, NUM_WATER_TOLERANCE> get_part_mut_drench( const bodypart_id &id ) const;

        float get_part_wetness_percentage( const bodypart_id &id ) const;

        const encumbrance_data &get_part_encumbrance_data( const bodypart_id &id )const;

        virtual void set_part_hp_cur( const bodypart_id &id, int set );
        void set_part_hp_max( const bodypart_id &id, int set );
        void set_part_healed_total( const bodypart_id &id, int set );
        void set_part_damage_disinfected( const bodypart_id &id, int set );
        void set_part_damage_bandaged( const bodypart_id &id, int set );

        void set_part_encumbrance_data( const bodypart_id &id, const encumbrance_data &set );

        void set_part_wetness( const bodypart_id &id, int set );
        void set_part_temp_cur( const bodypart_id &id, int set );
        void set_part_temp_conv( const bodypart_id &id, int set );
        void set_part_frostbite_timer( const bodypart_id &id, int set );

        void set_part_mut_drench( const bodypart_id &id, std::pair<water_tolerance, int> set );

        void mod_part_hp_cur( const bodypart_id &id, int mod );
        void mod_part_hp_max( const bodypart_id &id, int mod );
        void mod_part_healed_total( const bodypart_id &id, int mod );
        void mod_part_damage_disinfected( const bodypart_id &id, int mod );
        void mod_part_damage_bandaged( const bodypart_id &id, int mod );
        void mod_part_wetness( const bodypart_id &id, int mod );
        void mod_part_temp_cur( const bodypart_id &id, int mod );
        void mod_part_temp_conv( const bodypart_id &id, int mod );
        void mod_part_frostbite_timer( const bodypart_id &id, int mod );

        void set_all_parts_temp_cur( int set );
        void set_all_parts_temp_conv( int set );
        void set_all_parts_wetness( int set );
        void set_all_parts_hp_cur( int set );
        void set_all_parts_hp_to_max();

        bool has_atleast_one_wet_part() const;
        bool is_part_at_max_hp( const bodypart_id &id ) const;

        virtual int get_speed_base() const;
        virtual int get_speed_bonus() const;
        virtual int get_block_bonus() const;
        virtual int get_bash_bonus() const;
        virtual int get_cut_bonus() const;

        virtual float get_dodge_base() const = 0;
        virtual float get_hit_base() const = 0;
        virtual float get_dodge_bonus() const;
        virtual float get_hit_bonus() const;

        virtual float get_bash_mult() const;
        virtual float get_cut_mult() const;

        virtual bool get_melee_quiet() const;
        virtual bool has_grab_break_tec() const = 0;
        virtual int get_throw_resist() const;

        /*
         * Setters for stats and bonuses
         */
        virtual void mod_stat( const std::string &stat, float modifier );

        virtual void set_num_blocks_bonus( int nblocks );
        virtual void mod_num_blocks_bonus( int nblocks );
        virtual void mod_num_dodges_bonus( int ndodges );

        virtual void set_armor_res_bonus( int narm, const damage_type_id &dt );

        virtual void set_speed_base( int nspeed );
        virtual void set_speed_bonus( int nspeed );
        virtual void set_block_bonus( int nblock );
        virtual void set_bash_bonus( int nbash );
        virtual void set_cut_bonus( int ncut );

        virtual void mod_speed_bonus( int nspeed );
        virtual void mod_block_bonus( int nblock );
        virtual void mod_bash_bonus( int nbash );
        virtual void mod_cut_bonus( int ncut );
        virtual void mod_size_bonus( int nsize );

        virtual void set_dodge_bonus( float ndodge );
        virtual void set_hit_bonus( float nhit );

        virtual void mod_dodge_bonus( float ndodge );
        virtual void mod_hit_bonus( float  nhit );

        virtual void set_bash_mult( float nbashmult );
        virtual void set_cut_mult( float ncutmult );

        virtual void set_melee_quiet( bool nquiet );
        virtual void set_throw_resist( int nthrowres );

        virtual units::mass weight_capacity() const;

        /** Returns settings for pathfinding. */
        virtual const pathfinding_settings &get_pathfinding_settings() const = 0;
        /** Returns a set of points we do not want to path through. */
        virtual std::set<tripoint> get_path_avoid() const = 0;

        int moves;
        bool underwater;
        void draw( const catacurses::window &w, const point &origin, bool inverted ) const;
        void draw( const catacurses::window &w, const tripoint &origin, bool inverted ) const;
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
        virtual int print_info( const catacurses::window &w, int vStart, int vLines, int column ) const = 0;

        /** Describe this creature as seen by the avatar via infrared vision. */
        void describe_infrared( std::vector<std::string> &buf ) const;

        /** Describe this creature as detected by the avatar's special senses. */
        void describe_specials( std::vector<std::string> &buf ) const;

        // Message related stuff
        // These functions print to the sidebar message log. Unlike add_msg which prints messages
        // unconditionally, these only print messages when invoked for certain creature types:
        //
        // add_msg_if_player - only printed for avatar, not NPCs/monsters
        // add_msg_if_npc - only printed for NPCs, not players/monsters
        // add_msg_player_or_npc - printed for avatar or NPC, not monsters
        //
        // Examples:
        // add_msg_if_player( "You feel that you need to eat more calorie-dense food." );
        // add_msg_if_npc( "<npcname> falls off their mount!" );
        // add_msg_if_player_or_npc( "You open the door.", "<npcname> opens the door." );
        //
        virtual void add_msg_if_player( const std::string &/*msg*/ ) const {}
        virtual void add_msg_if_player( const game_message_params &/*params*/,
                                        const std::string &/*msg*/ ) const {}
        void add_msg_if_player( const translation &/*msg*/ ) const;
        void add_msg_if_player( const game_message_params &/*params*/, const translation &/*msg*/ ) const;
        template<typename ...Args>
        void add_msg_if_player( const char *const msg, Args &&... args ) const {
            return add_msg_if_player( string_format( msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_if_player( const std::string &msg, Args &&... args ) const {
            return add_msg_if_player( string_format( msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_if_player( const translation &msg, Args &&... args ) const {
            return add_msg_if_player( string_format( msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_if_player( const game_message_params &params, const char *const msg,
                                Args &&... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_if_player( params, string_format( msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_if_player( const game_message_params &params, const std::string &msg,
                                Args &&... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_if_player( params, string_format( msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_if_player( const game_message_params &params, const translation &msg,
                                Args &&... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_if_player( params, string_format( msg, std::forward<Args>( args )... ) );
        }

        virtual void add_msg_if_npc( const std::string &/*msg*/ ) const {}
        virtual void add_msg_if_npc( const game_message_params &/*params*/,
                                     const std::string &/*msg*/ ) const {}
        void add_msg_if_npc( const translation &/*msg*/ ) const;
        void add_msg_if_npc( const game_message_params &/*params*/, const translation &/*msg*/ ) const;
        template<typename ...Args>
        void add_msg_if_npc( const char *const msg, Args &&... args ) const {
            return add_msg_if_npc( string_format( msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_if_npc( const std::string &msg, Args &&... args ) const {
            return add_msg_if_npc( string_format( msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_if_npc( const translation &msg, Args &&... args ) const {
            return add_msg_if_npc( string_format( msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_if_npc( const game_message_params &params, const char *const msg,
                             Args &&... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_if_npc( params, string_format( msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_if_npc( const game_message_params &params, const std::string &msg,
                             Args &&... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_if_npc( params, string_format( msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_if_npc( const game_message_params &params, const translation &msg,
                             Args &&... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_if_npc( params, string_format( msg, std::forward<Args>( args )... ) );
        }

        virtual void add_msg_player_or_npc( const std::string &/*player_msg*/,
                                            const std::string &/*npc_msg*/ ) const {}
        virtual void add_msg_player_or_npc( const game_message_params &/*params*/,
                                            const std::string &/*player_msg*/,
                                            const std::string &/*npc_msg*/ ) const {}
        void add_msg_player_or_npc( const translation &/*player_msg*/,
                                    const translation &/*npc_msg*/ ) const;
        void add_msg_player_or_npc( const game_message_params &/*params*/,
                                    const translation &/*player_msg*/,
                                    const translation &/*npc_msg*/ ) const;
        template<typename ...Args>
        void add_msg_player_or_npc( const char *const player_msg, const char *const npc_msg,
                                    Args &&... args ) const {
            return add_msg_player_or_npc( string_format( player_msg, std::forward<Args>( args )... ),
                                          string_format( npc_msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_player_or_npc( const std::string &player_msg, const std::string &npc_msg,
                                    Args &&... args ) const {
            return add_msg_player_or_npc( string_format( player_msg, std::forward<Args>( args )... ),
                                          string_format( npc_msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_player_or_npc( const translation &player_msg, const translation &npc_msg,
                                    Args &&... args ) const {
            return add_msg_player_or_npc( string_format( player_msg, std::forward<Args>( args )... ),
                                          string_format( npc_msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_player_or_npc( const game_message_params &params, const char *const player_msg,
                                    const char *const npc_msg, Args &&... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_player_or_npc( params, string_format( player_msg, std::forward<Args>( args )... ),
                                          string_format( npc_msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_player_or_npc( const game_message_params &params, const std::string &player_msg,
                                    const std::string &npc_msg, Args &&... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_player_or_npc( params, string_format( player_msg, std::forward<Args>( args )... ),
                                          string_format( npc_msg, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_player_or_npc( const game_message_params &params, const translation &player_msg,
                                    const translation &npc_msg, Args &&... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_player_or_npc( params, string_format( player_msg, std::forward<Args>( args )... ),
                                          string_format( npc_msg, std::forward<Args>( args )... ) );
        }

        virtual void add_msg_debug_if_player( debugmode::debug_filter /*type*/,
                                              const std::string &/*msg*/ ) const {}
        template<typename ...Args>
        void add_msg_debug_if_player( debugmode::debug_filter type, const char *const msg,
                                      Args &&... args ) const {
            // expanding for string formatting can be expensive
            if( debug_mode ) {
                return add_msg_debug_if_player( type, string_format( msg, std::forward<Args>( args )... ) );
            }
        }
        template<typename ...Args>
        void add_msg_debug_if_player( debugmode::debug_filter type, const std::string &msg,
                                      Args &&... args ) const {
            if( debug_mode ) {
                return add_msg_debug_if_player( type, string_format( msg, std::forward<Args>( args )... ) );
            }
        }

        virtual void add_msg_debug_if_npc( debugmode::debug_filter /*type*/,
                                           const std::string &/*msg*/ ) const {}
        template<typename ...Args>
        void add_msg_debug_if_npc( debugmode::debug_filter type, const char *const msg,
                                   Args &&... args ) const {
            // expanding for string formatting can be expensive
            if( debug_mode ) {
                return add_msg_debug_if_npc( type, string_format( msg, std::forward<Args>( args )... ) );
            }
        }
        template<typename ...Args>
        void add_msg_debug_if_npc( debugmode::debug_filter type, const std::string &msg,
                                   Args &&... args ) const {
            if( debug_mode ) {
                return add_msg_debug_if_npc( type, string_format( msg, std::forward<Args>( args )... ) );
            }
        }

        virtual void add_msg_debug_player_or_npc( debugmode::debug_filter /*type*/,
                const std::string &/*player_msg*/,
                const std::string &/*npc_msg*/ ) const {}
        void add_msg_debug_player_or_npc( debugmode::debug_filter /*type*/,
                                          const translation &/*player_msg*/,
                                          const translation &/*npc_msg*/ ) const;
        template<typename ...Args>
        void add_msg_debug_player_or_npc( debugmode::debug_filter type, const char *const player_msg,
                                          const char *const npc_msg, Args &&... args ) const {
            // expanding for string formatting can be expensive
            if( debug_mode ) {
                return add_msg_debug_player_or_npc( type, string_format( player_msg,
                                                    std::forward<Args>( args )... ),
                                                    string_format( npc_msg, std::forward<Args>( args )... ) );
            }
        }
        template<typename ...Args>
        void add_msg_debug_player_or_npc( debugmode::debug_filter type, const std::string &player_msg,
                                          const std::string &npc_msg, Args &&... args ) const {
            if( debug_mode ) {
                return add_msg_debug_player_or_npc( type, string_format( player_msg,
                                                    std::forward<Args>( args )... ),
                                                    string_format( npc_msg, std::forward<Args>( args )... ) );
            }
        }

        virtual void add_msg_player_or_say( const std::string &/*player_msg*/,
                                            const std::string &/*npc_speech*/ ) const {}
        virtual void add_msg_player_or_say( const game_message_params &/*params*/,
                                            const std::string &/*player_msg*/,
                                            const std::string &/*npc_speech*/ ) const {}
        void add_msg_player_or_say( const translation &/*player_msg*/,
                                    const translation &/*npc_speech*/ ) const;
        void add_msg_player_or_say( const game_message_params &/*params*/,
                                    const translation &/*player_msg*/,
                                    const translation &/*npc_speech*/ ) const;
        template<typename ...Args>
        void add_msg_player_or_say( const char *const player_msg, const char *const npc_speech,
                                    Args &&... args ) const {
            return add_msg_player_or_say( string_format( player_msg, std::forward<Args>( args )... ),
                                          string_format( npc_speech, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_player_or_say( const std::string &player_msg, const std::string &npc_speech,
                                    Args &&... args ) const {
            return add_msg_player_or_say( string_format( player_msg, std::forward<Args>( args )... ),
                                          string_format( npc_speech, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_player_or_say( const translation &player_msg, const translation &npc_speech,
                                    Args &&... args ) const {
            return add_msg_player_or_say( string_format( player_msg, std::forward<Args>( args )... ),
                                          string_format( npc_speech, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_player_or_say( const game_message_params &params, const char *const player_msg,
                                    const char *const npc_speech, Args &&... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_player_or_say( params, string_format( player_msg, std::forward<Args>( args )... ),
                                          string_format( npc_speech, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_player_or_say( const game_message_params &params, const std::string &player_msg,
                                    const std::string &npc_speech, Args &&... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_player_or_say( params, string_format( player_msg, std::forward<Args>( args )... ),
                                          string_format( npc_speech, std::forward<Args>( args )... ) );
        }
        template<typename ...Args>
        void add_msg_player_or_say( const game_message_params &params, const translation &player_msg,
                                    const translation &npc_speech, Args &&... args ) const {
            if( params.type == m_debug && !debug_mode ) {
                return;
            }
            return add_msg_player_or_say( params, string_format( player_msg, std::forward<Args>( args )... ),
                                          string_format( npc_speech, std::forward<Args>( args )... ) );
        }

        virtual std::string extended_description() const = 0;

        /** Creature symbol background color */
        virtual nc_color symbol_color() const = 0;
        /** Creature symbol color */
        virtual nc_color basic_symbol_color() const = 0;
        /** Creature symbol */
        virtual const std::string &symbol() const = 0;
        virtual bool is_symbol_highlighted() const;

        std::unordered_map<std::string, std::string> &get_values();
        void clear_killer();
        // summoned creatures via spells
        void set_summon_time( const time_duration &length );
        // handles removing the creature if the timer runs out
        void decrement_summon_timer();
    protected:
        Creature *killer; // whoever killed us. this should be NULL unless we are dead
        void set_killer( Creature *killer );
        std::optional<time_point> lifespan_end = std::nullopt;
        /**
         * Processes one effect on the Creature.
         * Must not remove the effect, but can set it up for removal.
         */
        virtual void process_one_effect( effect &e, bool is_new ) = 0;

        pimpl<effects_map> effects;
        std::queue<scheduled_effect, std::list<scheduled_effect>> scheduled_effects;
        std::queue<terminating_effect, std::list<terminating_effect>> terminating_effects;

        std::vector<damage_over_time_data> damage_over_time_map;

        // Miscellaneous key/value pairs.
        std::unordered_map<std::string, std::string> values;

        // used for innate bonuses like effects. weapon bonuses will be
        // handled separately

        int num_blocks = 0; // base number of blocks/dodges per turn
        int num_dodges = 0;
        int num_blocks_bonus = 0; // bonus ""
        int num_dodges_bonus = 0;

        std::map<damage_type_id, float> armor_bonus;
        int speed_base = 0; // only speed needs a base, the rest are assumed at 0 and calculated off skills

        int speed_bonus = 0;
        float dodge_bonus = 0.0f;
        int block_bonus = 0;
        float hit_bonus = 0.0f;
        int bash_bonus = 0;
        int cut_bonus = 0;
        int size_bonus = 0;

        float bash_mult = 0.0f;
        float cut_mult = 0.0f;
        bool melee_quiet = false;

        int throw_resist = 0;

        time_point last_updated;

        bool fake = false;
        Creature();
        Creature( const Creature & );
        Creature( Creature && ) noexcept( map_is_noexcept );
        Creature &operator=( const Creature & );
        Creature &operator=( Creature && ) noexcept;

        virtual void on_stat_change( const std::string &, int ) {}
        virtual void on_effect_int_change( const efftype_id &, int, const bodypart_id & ) {}
        virtual void on_damage_of_type( const effect_source &, int, const damage_type_id &,
                                        const bodypart_id & ) {}

    public:
        // Keep a count of moves passed in which resets every 100 turns as a result of practicing archery proficiency
        // This is done this way in order to not destroy focus since `do_aim` is on a per-move basis.
        int archery_aim_counter = 0;

        // Select a bodypart depending on the attack's hitsize/limb restrictions
        bodypart_id select_body_part( int min_hit, int max_hit, bool can_attack_high, int hit_roll ) const;
        bodypart_id select_blocking_part( bool arm, bool leg, bool nonstandard ) const;
        bodypart_id random_body_part( bool main_parts_only = false ) const;
        std::vector<bodypart_id> get_all_eligable_parts( int min_hit, int max_hit,
                bool can_attack_high ) const;

        void add_damage_over_time( const damage_over_time_data &DoT );
        void process_damage_over_time();

        static void load_hit_range( const JsonObject & );
        // Empirically determined by "synthetic_range_test" in tests/ranged_balance.cpp.
        static std::vector <int> dispersion_for_even_chance_of_good_hit;
        /**
         * This function replaces the "<npcname>" substring with the @ref disp_name of this creature.
         *
         * Its purpose is to avoid repeated code and improve source readability / maintainability.
         *
         */
        std::string replace_with_npc_name( std::string input ) const;
        /**
         * Returns the location of the creature in map square coordinates (the most detailed
         * coordinate system), relative to a fixed global point of origin.
         */
        tripoint_abs_ms get_location() const;
        /**
         * Returns the location of the creature in global submap coordinates.
         */
        tripoint_abs_sm global_sm_location() const;
        /**
         * Returns the location of the creature in global overmap terrain coordinates.
         */
        tripoint_abs_omt global_omt_location() const;
    protected:
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
        void store( JsonOut &jsout ) const;
        // Load creature data from the given json object.
        void load( const JsonObject &jsin );

    private:
        int pain;
        // calculate how well the projectile hits
        double accuracy_projectile_attack( dealt_projectile_attack &attack ) const;
        // what bodypart does the projectile hit
        projectile_attack_results select_body_part_projectile_attack( const projectile &proj,
                double goodhit, double missed_by ) const;
        // do messaging and SCT for projectile hit
        void messaging_projectile_attack( const Creature *source,
                                          const projectile_attack_results &hit_selection, int total_damage ) const;
};
std::unique_ptr<talker> get_talker_for( Creature &me );
std::unique_ptr<talker> get_talker_for( const Creature &me );
std::unique_ptr<talker> get_talker_for( Creature *me );
#endif // CATA_SRC_CREATURE_H
