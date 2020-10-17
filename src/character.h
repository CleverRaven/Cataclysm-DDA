#pragma once
#ifndef CATA_SRC_CHARACTER_H
#define CATA_SRC_CHARACTER_H

#include <array>
#include <bitset>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "action.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character_id.h"
#include "character_martial_arts.h"
#include "color.h"
#include "creature.h"
#include "cursesdef.h"
#include "damage.h"
#include "enums.h"
#include "flat_set.h"
#include "game_constants.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "magic.h"
#include "magic_enchantment.h"
#include "memory_fast.h"
#include "monster.h"
#include "mtype.h"
#include "optional.h"
#include "pimpl.h"
#include "player_activity.h"
#include "pldata.h"
#include "point.h"
#include "ret_val.h"
#include "stomach.h"
#include "string_formatter.h"
#include "type_id.h"
#include "units.h"
#include "visitable.h"
#include "weighted_list.h"

class JsonIn;
class JsonObject;
class JsonOut;
class SkillLevel;
class SkillLevelMap;
class bionic_collection;
class faction;
class ma_technique;
class player;
class player_morale;
class vehicle;
struct bionic;
struct construction;
struct dealt_projectile_attack;
struct islot_comestible;
struct itype;
struct mutation_branch;
struct needs_rates;
struct pathfinding_settings;
struct points_left;
struct trap;
template <typename E> struct enum_traits;

using drop_location = std::pair<item_location, int>;
using drop_locations = std::list<drop_location>;

#define MAX_CLAIRVOYANCE 40

enum vision_modes {
    DEBUG_NIGHTVISION,
    NV_GOGGLES,
    BIRD_EYE,
    URSINE_VISION,
    BOOMERED,
    DARKNESS,
    IR_VISION,
    VISION_CLAIRVOYANCE,
    VISION_CLAIRVOYANCE_PLUS,
    VISION_CLAIRVOYANCE_SUPER,
    NUM_VISION_MODES
};

enum character_movemode : int {
    CMM_WALK = 0,
    CMM_RUN,
    CMM_CROUCH,
    CMM_COUNT
};

template<>
struct enum_traits<character_movemode> {
    static constexpr auto last = character_movemode::CMM_COUNT;
};

enum class fatigue_levels : int {
    tired = 191,
    dead_tired = 383,
    exhausted = 575,
    massive = 1000
};

constexpr inline bool operator>=( const fatigue_levels &lhs, const fatigue_levels &rhs )
{
    return static_cast<int>( lhs ) >= static_cast<int>( rhs );
}

constexpr inline bool operator<( const fatigue_levels &lhs, const fatigue_levels &rhs )
{
    return static_cast<int>( lhs ) < static_cast<int>( rhs );
}

template<typename T>
constexpr inline bool operator>=( const T &lhs, const fatigue_levels &rhs )
{
    return lhs >= static_cast<T>( rhs );
}

template<typename T>
constexpr inline bool operator>( const T &lhs, const fatigue_levels &rhs )
{
    return lhs > static_cast<T>( rhs );
}

template<typename T>
constexpr inline bool operator<=( const T &lhs, const fatigue_levels &rhs )
{
    return lhs <= static_cast<T>( rhs );
}

template<typename T>
constexpr inline bool operator<( const T &lhs, const fatigue_levels &rhs )
{
    return lhs < static_cast<T>( rhs );
}

template<typename T>
constexpr inline int operator/( const fatigue_levels &lhs, const T &rhs )
{
    return static_cast<T>( lhs ) / rhs;
}

template<typename T>
constexpr inline int operator+( const fatigue_levels &lhs, const T &rhs )
{
    return static_cast<T>( lhs ) + rhs;
}

template<typename T>
constexpr inline int operator-( const fatigue_levels &lhs, const T &rhs )
{
    return static_cast<T>( lhs ) - rhs;
}

template<typename T>
constexpr inline int operator-( const T &lhs, const fatigue_levels &rhs )
{
    return lhs - static_cast<T>( rhs );
}

const std::unordered_map<std::string, fatigue_levels> fatigue_level_strs = { {
        { "TIRED", fatigue_levels::tired },
        { "DEAD_TIRED", fatigue_levels::dead_tired },
        { "EXHAUSTED", fatigue_levels::exhausted },
        { "MASSIVE_FATIGUE", fatigue_levels::massive }
    }
};

// Sleep deprivation is defined in minutes, and although most calculations scale linearly,
// maluses are bestowed only upon reaching the tiers defined below.
enum class sleep_deprivation_levels : int {
    harmless = 2 * 24 * 60,
    minor = 4 * 24 * 60,
    serious = 7 * 24 * 60,
    major = 10 * 24 * 60,
    massive = 14 * 24 * 60
};

constexpr inline bool operator>=( const sleep_deprivation_levels &lhs,
                                  const sleep_deprivation_levels &rhs )
{
    return static_cast<int>( lhs ) >= static_cast<int>( rhs );
}

constexpr inline bool operator<( const sleep_deprivation_levels &lhs,
                                 const sleep_deprivation_levels &rhs )
{
    return static_cast<int>( lhs ) < static_cast<int>( rhs );
}

template<typename T>
constexpr inline bool operator>=( const T &lhs, const sleep_deprivation_levels &rhs )
{
    return lhs >= static_cast<T>( rhs );
}

template<typename T>
constexpr inline bool operator>( const T &lhs, const sleep_deprivation_levels &rhs )
{
    return lhs > static_cast<T>( rhs );
}

template<typename T>
constexpr inline bool operator<=( const T &lhs, const sleep_deprivation_levels &rhs )
{
    return lhs <= static_cast<T>( rhs );
}

template<typename T>
constexpr inline bool operator<( const T &lhs, const sleep_deprivation_levels &rhs )
{
    return lhs < static_cast<T>( rhs );
}

template<typename T>
constexpr inline int operator/( const sleep_deprivation_levels &lhs, const T &rhs )
{
    return static_cast<T>( lhs ) / rhs;
}

template<typename T>
constexpr inline int operator+( const sleep_deprivation_levels &lhs, const T &rhs )
{
    return static_cast<T>( lhs ) + rhs;
}

template<typename T>
constexpr inline int operator-( const sleep_deprivation_levels &lhs, const T &rhs )
{
    return static_cast<T>( lhs ) - rhs;
}

template<typename T>
constexpr inline int operator-( const T &lhs, const sleep_deprivation_levels &rhs )
{
    return lhs - static_cast<T>( rhs );
}

// This tries to represent both rating and
// character's decision to respect said rating
enum class edible_rating {
    // Edible or we pretend it is
    edible,
    // Not food at all
    inedible,
    // Not food because mutated mouth/system
    inedible_mutation,
    // You can eat it, but it will hurt morale
    allergy,
    // Smaller allergy penalty
    allergy_weak,
    // Cannibalism (unless psycho/cannibal)
    cannibalism,
    // Rotten or not rotten enough (for saprophages)
    rotten,
    // Can provoke vomiting if you already feel nauseous.
    nausea,
    // We did overeat, cramming more will surely cause vomiting.
    bloated,
    // We can eat this, but we'll overeat
    too_full,
    // Some weird stuff that requires a tool we don't have
    no_tool
};

enum class rechargeable_cbm {
    none = 0,
    reactor,
    furnace,
    other
};

struct layer_details {

    std::vector<int> pieces;
    int max = 0;
    int total = 0;

    void reset();
    int layer( int encumbrance );

    bool operator ==( const layer_details &rhs ) const {
        return max == rhs.max &&
               total == rhs.total &&
               pieces == rhs.pieces;
    }
};

struct encumbrance_data {
    int encumbrance = 0;
    int armor_encumbrance = 0;
    int layer_penalty = 0;

    std::array<layer_details, static_cast<size_t>( layer_level::MAX_CLOTHING_LAYER )>
    layer_penalty_details;

    void layer( const layer_level level, const int encumbrance ) {
        layer_penalty += layer_penalty_details[static_cast<size_t>( level )].layer( encumbrance );
    }

    void reset() {
        *this = encumbrance_data();
    }

    bool operator ==( const encumbrance_data &rhs ) const {
        return encumbrance == rhs.encumbrance &&
               armor_encumbrance == rhs.armor_encumbrance &&
               layer_penalty == rhs.layer_penalty &&
               layer_penalty_details == rhs.layer_penalty_details;
    }
};

struct aim_type {
    std::string name;
    std::string action;
    std::string help;
    bool has_threshold;
    int threshold;
};

struct special_attack {
    std::string text;
    damage_instance damage;
};

struct social_modifiers {
    int lie = 0;
    int persuade = 0;
    int intimidate = 0;

    social_modifiers &operator+=( const social_modifiers &other ) {
        this->lie += other.lie;
        this->persuade += other.persuade;
        this->intimidate += other.intimidate;
        return *this;
    }
};

struct consumption_event {
    time_point time;
    itype_id type_id;
    uint64_t component_hash;

    consumption_event() = default;
    consumption_event( const item &food ) : time( calendar::turn ) {
        type_id = food.typeId();
        component_hash = food.make_component_hash();
    }
    void serialize( JsonOut &json ) const;
    void deserialize( JsonIn &jsin );
};

inline social_modifiers operator+( social_modifiers lhs, const social_modifiers &rhs )
{
    lhs += rhs;
    return lhs;
}

class Character : public Creature, public visitable<Character>
{
    public:
        Character( const Character & ) = delete;
        Character &operator=( const Character & ) = delete;
        ~Character() override;

        Character *as_character() override {
            return this;
        }
        const Character *as_character() const override {
            return this;
        }

        character_id getID() const;
        // sets the ID, will *only* succeed when the current id is not valid
        // allows forcing a -1 id which is required for templates to not throw errors
        void setID( character_id i, bool force = false );

        field_type_id bloodType() const override;
        field_type_id gibType() const override;
        bool is_warm() const override;
        bool in_species( const species_id &spec ) const override;
        // Turned to false for simulating NPCs on distant missions so they don't drop all their gear in sight
        bool death_drops;
        const std::string &symbol() const override;

        enum class comfort_level {
            impossible = -999,
            uncomfortable = -7,
            neutral = 0,
            slightly_comfortable = 3,
            comfortable = 5,
            very_comfortable = 10
        };

        enum stat {
            STRENGTH,
            DEXTERITY,
            INTELLIGENCE,
            PERCEPTION,
            DUMMY_STAT
        };

        // Character stats
        // TODO: Make those protected
        int str_max;
        int dex_max;
        int int_max;
        int per_max;

        int str_cur;
        int dex_cur;
        int int_cur;
        int per_cur;

        // The prevalence of getter, setter, and mutator functions here is partially
        // a result of the slow, piece-wise migration of the player class upwards into
        // the character class. As enough logic is moved upwards to fully separate
        // utility upwards out of the player class, as many of these as possible should
        // be eliminated to allow for proper code separation. (Note: Not "all", many").
        /** Getters for stats exclusive to characters */
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

        int get_speed() const override;

        // Penalty modifiers applied for ranged attacks due to low stats
        virtual int ranged_dex_mod() const;
        virtual int ranged_per_mod() const;

        /** Setters for stats exclusive to characters */
        virtual void set_str_bonus( int nstr );
        virtual void set_dex_bonus( int ndex );
        virtual void set_per_bonus( int nper );
        virtual void set_int_bonus( int nint );
        virtual void mod_str_bonus( int nstr );
        virtual void mod_dex_bonus( int ndex );
        virtual void mod_per_bonus( int nper );
        virtual void mod_int_bonus( int nint );

        // Prints message(s) about current health
        void print_health() const;

        /** Getters for health values exclusive to characters */
        virtual int get_healthy() const;
        virtual int get_healthy_mod() const;

        /** Modifiers for health values exclusive to characters */
        virtual void mod_healthy( int nhealthy );
        virtual void mod_healthy_mod( int nhealthy_mod, int cap );

        /** Setters for health values exclusive to characters */
        virtual void set_healthy( int nhealthy );
        virtual void set_healthy_mod( int nhealthy_mod );

        /** Getter for need values exclusive to characters */
        int get_stored_kcal() const;
        int get_healthy_kcal() const;
        // Maximum stored calories, excluding stomach.
        // If more would be digested, it is instead wasted.
        int max_stored_calories() const;
        float get_kcal_percent() const;
        float get_hunger() const;
        int get_thirst() const;
        std::pair<std::string, nc_color> get_thirst_description() const;
        std::pair<std::string, nc_color> get_hunger_description() const;
        std::pair<std::string, nc_color> get_fatigue_description() const;
        int get_fatigue() const;
        int get_sleep_deprivation() const;

        std::pair<std::string, nc_color> get_pain_description() const override;

        /** Modifiers for need values exclusive to characters */
        virtual void mod_stored_kcal( int nkcal );
        virtual void mod_stored_nutr( int nnutr );
        virtual void mod_hunger( float nhunger );
        virtual void mod_thirst( int nthirst );
        virtual void mod_fatigue( int nfatigue );
        virtual void mod_sleep_deprivation( int nsleep_deprivation );

        /** Setters for need values exclusive to characters */
        virtual void set_stored_kcal( int kcal );
        virtual void set_hunger( float nhunger );
        virtual void set_thirst( int nthirst );
        virtual void set_fatigue( int nfatigue );
        virtual void set_sleep_deprivation( int nsleep_deprivation );

        void mod_stat( const std::string &stat, float modifier ) override;

        /** Get size class of character **/
        m_size get_size() const override;

        /** Returns either "you" or the player's name. capitalize_first assumes
            that the character's name is already upper case and uses it only for
            possessive "your" and "you"
        **/
        std::string disp_name( bool possessive = false, bool capitalize_first = false ) const override;
        /** Returns the name of the player's outer layer, e.g. "armor plates" */
        std::string skin_name() const override;

        /* returns the character's faction */
        virtual faction *get_faction() const {
            return nullptr;
        }
        void set_fac_id( const std::string &my_fac_id );

        /* Adjusts provided sight dispersion to account for player stats */
        int effective_dispersion( int dispersion ) const;

        /* Accessors for aspects of aim speed. */
        std::vector<aim_type> get_aim_types( const item &gun ) const;
        std::pair<int, int> get_fastest_sight( const item &gun, double recoil ) const;
        int get_most_accurate_sight( const item &gun ) const;
        double aim_speed_skill_modifier( const skill_id &gun_skill ) const;
        double aim_speed_dex_modifier() const;
        double aim_speed_encumbrance_modifier() const;
        double aim_cap_from_volume( const item &gun ) const;

        /* Calculate aim improvement per move spent aiming at a given @ref recoil */
        double aim_per_move( const item &gun, double recoil ) const;

        /** Combat getters */
        float get_dodge_base() const override;
        float get_hit_base() const override;

        const tripoint &pos() const override;
        /** Returns the player's sight range */
        int sight_range( int light_level ) const override;
        /** Returns the player maximum vision range factoring in mutations, diseases, and other effects */
        int  unimpaired_range() const;
        /** Returns true if overmap tile is within player line-of-sight */
        bool overmap_los( const tripoint &omt, int sight_points );
        /** Returns the distance the player can see on the overmap */
        int  overmap_sight_range( int light_level ) const;
        /** Returns the distance the player can see through walls */
        int  clairvoyance() const;
        /** Returns true if the player has some form of impaired sight */
        bool sight_impaired() const;
        /** Returns true if the player or their vehicle has an alarm clock */
        bool has_alarm_clock() const;
        /** Returns true if the player or their vehicle has a watch */
        bool has_watch() const;
        /** Called after every action, invalidates player caches */
        void action_taken();
        /** Returns true if the player is knocked over or has broken legs */
        bool is_on_ground() const override;
        /** Returns the player's speed for swimming across water tiles */
        int  swim_speed() const;
        /**
         * Adds a reason for why the player would miss a melee attack.
         *
         * To possibly be messaged to the player when he misses a melee attack.
         * @param reason A message for the player that gives a reason for him missing.
         * @param weight The weight used when choosing what reason to pick when the
         * player misses.
         */
        void add_miss_reason( const std::string &reason, unsigned int weight );
        /** Clears the list of reasons for why the player would miss a melee attack. */
        void clear_miss_reasons();
        /**
         * Returns an explanation for why the player would miss a melee attack.
         */
        std::string get_miss_reason();

        /**
          * Handles passive regeneration of pain and maybe hp.
          */
        void regen( int rate_multiplier );
        // called once per 24 hours to enforce the minimum of 1 hp healed per day
        // TODO: Move to Character once heal() is moved
        void enforce_minimum_healing();
        /** get best quality item that this character has */
        item *best_quality_item( const quality_id &qual );
        /** Handles health fluctuations over time */
        virtual void update_health( int external_modifiers = 0 );
        /** Updates all "biology" by one turn. Should be called once every turn. */
        void update_body();
        /** Updates all "biology" as if time between `from` and `to` passed. */
        void update_body( const time_point &from, const time_point &to );
        /** Updates the stomach to give accurate hunger messages */
        void update_stomach( const time_point &from, const time_point &to );
        /** Increases hunger, thirst, fatigue and stimulants wearing off. `rate_multiplier` is for retroactive updates. */
        void update_needs( int rate_multiplier );
        needs_rates calc_needs_rates() const;
        /** Kills the player if too hungry, stimmed up etc., forces tired player to sleep and prints warnings. */
        void check_needs_extremes();
        /** Returns if the player has hibernation mutation and is asleep and well fed */
        bool is_hibernating() const;
        /** Maintains body temperature */
        void update_bodytemp();

        /** Equalizes heat between body parts */
        void temp_equalizer( body_part bp1, body_part bp2 );

        struct comfort_response_t {
            comfort_level level = comfort_level::neutral;
            const item *aid = nullptr;
        };
        /** Rate point's ability to serve as a bed. Only takes certain mutations into account, and not fatigue nor stimulants. */
        comfort_response_t base_comfort_value( const tripoint &p ) const;

        /** Define blood loss (in percents) */
        int blood_loss( body_part bp ) const;

        /** Resets the value of all bonus fields to 0. */
        void reset_bonuses() override;
        /** Resets stats, and applies effects in an idempotent manner */
        void reset_stats() override;
        /** Handles stat and bonus reset. */
        void reset() override;

        /** Recalculates encumbrance cache. */
        void reset_encumbrance();
        /** Returns ENC provided by armor, etc. */
        int encumb( body_part bp ) const;

        /** Returns body weight plus weight of inventory and worn/wielded items */
        units::mass get_weight() const override;
        /** Get encumbrance for all body parts. */
        std::array<encumbrance_data, num_bp> get_encumbrance() const;
        /** Get encumbrance for all body parts as if `new_item` was also worn. */
        std::array<encumbrance_data, num_bp> get_encumbrance( const item &new_item ) const;
        /** Get encumbrance penalty per layer & body part */
        int extraEncumbrance( layer_level level, int bp ) const;

        /** Returns true if the character is wearing power armor */
        bool is_wearing_power_armor( bool *hasHelmet = nullptr ) const;
        /** Returns true if the character is wearing active power */
        bool is_wearing_active_power_armor() const;
        /** Returns true if the player is wearing an active optical cloak */
        bool is_wearing_active_optcloak() const;

        /** Returns true if the player is in a climate controlled area or armor */
        bool in_climate_control();

        /** Returns wind resistance provided by armor, etc **/
        int get_wind_resistance( body_part bp ) const;

        /** Returns true if the player isn't able to see */
        bool is_blind() const;

        bool is_invisible() const;
        /** Checks is_invisible() as well as other factors */
        int visibility( bool check_color = false, int stillness = 0 ) const;

        /** Returns character luminosity based on the brightest active item they are carrying */
        float active_light() const;

        bool sees_with_specials( const Creature &critter ) const;

        /** Bitset of all the body parts covered only with items with `flag` (or nothing) */
        body_part_set exclusive_flag_coverage( const std::string &flag ) const;

        /** Processes effects which may prevent the Character from moving (bear traps, crushed, etc.).
         *  Returns false if movement is stopped. */
        bool move_effects( bool attacking ) override;
        /** Check against the character's current movement mode */
        bool movement_mode_is( character_movemode mode ) const;
        character_movemode get_movement_mode() const;

        virtual void set_movement_mode( character_movemode mode ) = 0;

        /** Performs any Character-specific modifications to the arguments before passing to Creature::add_effect(). */
        void add_effect( const efftype_id &eff_id, const time_duration &dur, body_part bp = num_bp,
                         bool permanent = false,
                         int intensity = 0, bool force = false, bool deferred = false ) override;

        /**Determine if character is susceptible to dis_type and if so apply the symptoms*/
        void expose_to_disease( diseasetype_id dis_type );
        /**
         * Handles end-of-turn processing.
         */
        void process_turn() override;

        /** Recalculates HP after a change to max strength */
        void recalc_hp();
        /** Modifies the player's sight values
         *  Must be called when any of the following change:
         *  This must be called when any of the following change:
         * - effects
         * - bionics
         * - traits
         * - underwater
         * - clothes
         */
        void recalc_sight_limits();
        /**
         * Returns the apparent light level at which the player can see.
         * This is adjusted by the light level at the *character's* position
         * to simulate glare, etc, night vision only works if you are in the dark.
         */
        float get_vision_threshold( float light_level ) const;
        /**
         * Flag encumbrance for updating.
        */
        void flag_encumbrance();
        /**
         * Checks worn items for the "RESET_ENCUMBRANCE" flag, which indicates
         * that encumbrance may have changed and require recalculating.
         */
        void check_item_encumbrance_flag();

        /** Returns true if the character is wearing something on the entered body_part, ignoring items with the ALLOWS_NATURAL_ATTACKS flag */
        bool natural_attack_restricted_on( body_part bp ) const;

        int blocks_left;
        int dodges_left;

        double recoil = MAX_RECOIL;

        /** Returns true if the player is able to use a miss recovery technique */
        bool can_miss_recovery( const item &weap ) const;
        /** Returns true if the player has quiet melee attacks */
        bool is_quiet() const;

        // melee.cpp
        /** Checks for valid block abilities and reduces damage accordingly. Returns true if the player blocks */
        bool block_hit( Creature *source, body_part &bp_hit, damage_instance &dam ) override;
        /** Returns the best item for blocking with */
        item &best_shield();
        /** Calculates melee weapon wear-and-tear through use, returns true if item is destroyed. */
        bool handle_melee_wear( item &shield, float wear_multiplier = 1.0f );
        /** Returns a random valid technique */
        matec_id pick_technique( Creature &t, const item &weap,
                                 bool crit, bool dodge_counter, bool block_counter );
        void perform_technique( const ma_technique &technique, Creature &t, damage_instance &di,
                                int &move_cost );
        /**
         * Sets up a melee attack and handles melee attack function calls
         * @param t Creature to attack
         * @param allow_special whether non-forced martial art technique or mutation attack should be
         *   possible with this attack.
         * @param force_technique special technique to use in attack.
         * @param allow_unarmed always uses the wielded weapon regardless of martialarts style
         */
        void melee_attack( Creature &t, bool allow_special, const matec_id &force_technique,
                           bool allow_unarmed = true );
        /**
         * Calls the to other melee_attack function with an empty technique id (meaning no specific
         * technique should be used).
         */
        void melee_attack( Creature &t, bool allow_special );
        /** Handles combat effects, returns a string of any valid combat effect messages */
        std::string melee_special_effects( Creature &t, damage_instance &d, item &weap );
        /** Performs special attacks and their effects (poisonous, stinger, etc.) */
        void perform_special_attacks( Creature &t, dealt_damage_instance &dealt_dam );
        bool reach_attacking = false;

        /** Returns a vector of valid mutation attacks */
        std::vector<special_attack> mutation_attacks( Creature &t ) const;
        /** Returns the bonus bashing damage the player deals based on their stats */
        float bonus_damage( bool random ) const;
        /** Returns weapon skill */
        float get_melee_hit_base() const;
        /** Returns the player's basic hit roll that is compared to the target's dodge roll */
        float hit_roll() const override;
        /** Returns the chance to critical given a hit roll and target's dodge roll */
        double crit_chance( float roll_hit, float target_dodge, const item &weap ) const;
        /** Returns true if the player scores a critical hit */
        bool scored_crit( float target_dodge, const item &weap ) const;
        /** Returns cost (in moves) of attacking with given item (no modifiers, like stuck) */
        int attack_speed( const item &weap ) const;
        /** Gets melee accuracy component from weapon+skills */
        float get_hit_weapon( const item &weap ) const;

        // If average == true, adds expected values of random rolls instead of rolling.
        /** Adds all 3 types of physical damage to instance */
        void roll_all_damage( bool crit, damage_instance &di, bool average, const item &weap ) const;
        /** Adds player's total bash damage to the damage instance */
        void roll_bash_damage( bool crit, damage_instance &di, bool average, const item &weap ) const;
        /** Adds player's total cut damage to the damage instance */
        void roll_cut_damage( bool crit, damage_instance &di, bool average, const item &weap ) const;
        /** Adds player's total stab damage to the damage instance */
        void roll_stab_damage( bool crit, damage_instance &di, bool average, const item &weap ) const;

    private:
        /** Check if an area-of-effect technique has valid targets */
        bool valid_aoe_technique( Creature &t, const ma_technique &technique );
        bool valid_aoe_technique( Creature &t, const ma_technique &technique,
                                  std::vector<Creature *> &targets );
    public:

        // any side effects that might happen when the Character is hit
        void on_hit( Creature *source, bodypart_id /*bp_hit*/,
                     float /*difficulty*/, dealt_projectile_attack const * /*proj*/ ) override;
        // any side effects that might happen when the Character hits a Creature
        void did_hit( Creature &target );

        /** Actually hurt the player, hurts a body_part directly, no armor reduction */
        void apply_damage( Creature *source, bodypart_id hurt, int dam,
                           bool bypass_med = false ) override;
        /** Calls Creature::deal_damage and handles damaged effects (waking up, etc.) */
        dealt_damage_instance deal_damage( Creature *source, bodypart_id bp,
                                           const damage_instance &d ) override;
        /** Reduce healing effect intensity, return initial intensity of the effect */
        int reduce_healing_effect( const efftype_id &eff_id, int remove_med, body_part hurt );

        void cough( bool harmful = false, int loudness = 4 );
        /**
         * Check for relevant passive, non-clothing that can absorb damage, and reduce by specified
         * damage unit.  Only flat bonuses are checked here.  Multiplicative ones are checked in
         * @ref player::absorb_hit.  The damage amount will never be reduced to less than 0.
         * This is called from @ref player::absorb_hit
         */
        void passive_absorb_hit( body_part bp, damage_unit &du ) const;
        /** Runs through all bionics and armor on a part and reduces damage through their armor_absorb */
        void absorb_hit( body_part bp, damage_instance &dam ) override;
        /**
         * Reduces and mutates du, prints messages about armor taking damage.
         * @return true if the armor was completely destroyed (and the item must be deleted).
         */
        bool armor_absorb( damage_unit &du, item &armor );
        /**
         * Check for passive bionics that provide armor, and returns the armor bonus
         * This is called from player::passive_absorb_hit
         */
        float bionic_armor_bonus( body_part bp, damage_type dt ) const;
        /** Returns the armor bonus against given type from martial arts buffs */
        int mabuff_armor_bonus( damage_type type ) const;
        /** Returns overall fire resistance for the body part */
        int get_armor_fire( body_part bp ) const;
        // --------------- Mutation Stuff ---------------
        // In newcharacter.cpp
        /** Returns the id of a random starting trait that costs >= 0 points */
        trait_id random_good_trait();
        /** Returns the id of a random starting trait that costs < 0 points */
        trait_id random_bad_trait();

        // In mutation.cpp
        /** Returns true if the player has the entered trait */
        bool has_trait( const trait_id &b ) const override;
        /** Returns true if the player has the entered starting trait */
        bool has_base_trait( const trait_id &b ) const;
        /** Returns true if player has a trait with a flag */
        bool has_trait_flag( const std::string &b ) const;
        /** Returns the trait id with the given invlet, or an empty string if no trait has that invlet */
        trait_id trait_by_invlet( int ch ) const;

        /** Toggles a trait on the player and in their mutation list */
        void toggle_trait( const trait_id & );
        /** Add or removes a mutation on the player, but does not trigger mutation loss/gain effects. */
        void set_mutation( const trait_id & );
        void unset_mutation( const trait_id & );
        /**Unset switched mutation and set target mutation instead*/
        void switch_mutations( const trait_id &switched, const trait_id &target, bool start_powered );

        // Trigger and disable mutations that can be so toggled.
        void activate_mutation( const trait_id &mutation );
        void deactivate_mutation( const trait_id &mut );

        /** Converts a body_part to an hp_part */
        static hp_part bp_to_hp( body_part bp );
        /** Converts an hp_part to a body_part */
        static body_part hp_to_bp( hp_part hpart );

        bool can_mount( const monster &critter ) const;
        void mount_creature( monster &z );
        bool is_mounted() const;
        bool check_mount_will_move( const tripoint &dest_loc );
        bool check_mount_is_spooked();
        void dismount();
        void forced_dismount();

        bool is_deaf() const;
        /** Returns true if the player has two functioning arms */
        bool has_two_arms() const;
        /** Returns the number of functioning arms */
        int get_working_arm_count() const;
        /** Returns the number of functioning legs */
        int get_working_leg_count() const;
        /** Returns true if the limb is disabled(12.5% or less hp)*/
        bool is_limb_disabled( hp_part limb ) const;
        /** Returns true if the limb is hindered(40% or less hp) */
        bool is_limb_hindered( hp_part limb ) const;
        /** Returns true if the limb is broken */
        bool is_limb_broken( hp_part limb ) const;
        /** source of truth of whether a Character can run */
        bool can_run();
        /** Hurts all body parts for dam, no armor reduction */
        void hurtall( int dam, Creature *source, bool disturb = true );
        /** Harms all body parts for dam, with armor reduction. If vary > 0 damage to parts are random within vary % (1-100) */
        int hitall( int dam, int vary, Creature *source );
        /** Handles effects that happen when the player is damaged and aware of the fact. */
        void on_hurt( Creature *source, bool disturb = true );
        /** Heals a body_part for dam */
        void heal( body_part healed, int dam );
        /** Heals an hp_part for dam */
        void heal( hp_part healed, int dam );
        /** Heals all body parts for dam */
        void healall( int dam );
        /**
         * Displays menu with body part hp, optionally with hp estimation after healing.
         * Returns selected part.
         * menu_header - name of item that triggers this menu
         * show_all - show and enable choice of all limbs, not only healable
         * precise - show numerical hp
         * normal_bonus - heal normal limb
         * head_bonus - heal head
         * torso_bonus - heal torso
         * bleed - chance to stop bleeding
         * bite - chance to remove bite
         * infect - chance to remove infection
         * bandage_power - quality of bandage
         * disinfectant_power - quality of disinfectant
         */
        hp_part body_window( const std::string &menu_header,
                             bool show_all, bool precise,
                             int normal_bonus, int head_bonus, int torso_bonus,
                             float bleed, float bite, float infect, float bandage_power, float disinfectant_power ) const;

        // Returns color which this limb would have in healing menus
        nc_color limb_color( body_part bp, bool bleed, bool bite, bool infect ) const;

        static const std::vector<material_id> fleshy;
        bool made_of( const material_id &m ) const override;
        bool made_of_any( const std::set<material_id> &ms ) const override;

        // Drench cache
        enum water_tolerance {
            WT_IGNORED = 0,
            WT_NEUTRAL,
            WT_GOOD,
            NUM_WATER_TOLERANCE
        };
        inline int posx() const override {
            return position.x;
        }
        inline int posy() const override {
            return position.y;
        }
        inline int posz() const override {
            return position.z;
        }
        inline void setx( int x ) {
            setpos( tripoint( x, position.y, position.z ) );
        }
        inline void sety( int y ) {
            setpos( tripoint( position.x, y, position.z ) );
        }
        inline void setz( int z ) {
            setpos( tripoint( position.xy(), z ) );
        }
        inline void setpos( const tripoint &p ) override {
            position = p;
        }

        /**
         * Global position, expressed in map square coordinate system
         * (the most detailed coordinate system), used by the @ref map.
         */
        virtual tripoint global_square_location() const;
        /**
        * Returns the location of the player in global submap coordinates.
        */
        tripoint global_sm_location() const;
        /**
        * Returns the location of the player in global overmap terrain coordinates.
        */
        tripoint global_omt_location() const;

    private:
        /** Retrieves a stat mod of a mutation. */
        int get_mod( const trait_id &mut, const std::string &arg ) const;
        /** Applies skill-based boosts to stats **/
        void apply_skill_boost();
    protected:
        void do_skill_rust();
        /** Applies stat mods to character. */
        void apply_mods( const trait_id &mut, bool add_remove );

        /** Recalculate encumbrance for all body parts. */
        std::array<encumbrance_data, num_bp> calc_encumbrance() const;
        /** Recalculate encumbrance for all body parts as if `new_item` was also worn. */
        std::array<encumbrance_data, num_bp> calc_encumbrance( const item &new_item ) const;

        /** Applies encumbrance from mutations and bionics only */
        void mut_cbm_encumb( std::array<encumbrance_data, num_bp> &vals ) const;

        /** Return the position in the worn list where new_item would be
         * put by default */
        std::list<item>::iterator position_to_wear_new_item( const item &new_item );

        /** Applies encumbrance from items only
         * If new_item is not null, then calculate under the asumption that it
         * is added to existing work items. */
        void item_encumb( std::array<encumbrance_data, num_bp> &vals,
                          const item &new_item ) const;

        std::array<std::array<int, NUM_WATER_TOLERANCE>, num_bp> mut_drench;

    public:
        // recalculates enchantment cache by iterating through all held, worn, and wielded items
        void recalculate_enchantment_cache();
        // gets add and mult value from enchantment cache
        double calculate_by_enchantment( double modify, enchantment::mod value,
                                         bool round_output = false ) const;

        /** Returns true if the player has any martial arts buffs attached */
        bool has_mabuff( const mabuff_id &buff_id ) const;
        /** Returns true if the player has a grab breaking technique available */
        bool has_grab_break_tec() const override {
            return martial_arts_data.has_grab_break_tec();
        }

        /** Returns the to hit bonus from martial arts buffs */
        float mabuff_tohit_bonus() const;
        /** Returns the dodge bonus from martial arts buffs */
        float mabuff_dodge_bonus() const;
        /** Returns the block bonus from martial arts buffs */
        int mabuff_block_bonus() const;
        /** Returns the speed bonus from martial arts buffs */
        int mabuff_speed_bonus() const;
        /** Returns the damage multiplier to given type from martial arts buffs */
        float mabuff_damage_mult( damage_type type ) const;
        /** Returns the flat damage bonus to given type from martial arts buffs, applied after the multiplier */
        int mabuff_damage_bonus( damage_type type ) const;
        /** Returns the flat penalty to move cost of attacks. If negative, that's a bonus. Applied after multiplier. */
        int mabuff_attack_cost_penalty() const;
        /** Returns the multiplier on move cost of attacks. */
        float mabuff_attack_cost_mult() const;

        /** Handles things like destruction of armor, etc. */
        void mutation_effect( const trait_id &mut );
        /** Handles what happens when you lose a mutation. */
        void mutation_loss_effect( const trait_id &mut );

        bool has_active_mutation( const trait_id &b ) const;
    private:
        // The old mutation algorithm
        void old_mutate();
    public:
        /** Picks a random valid mutation and gives it to the Character, possibly removing/changing others along the way */
        void mutate();
        /** Returns true if the player doesn't have the mutation or a conflicting one and it complies with the force typing */
        bool mutation_ok( const trait_id &mutation, bool force_good, bool force_bad ) const;
        /** Picks a random valid mutation in a category and mutate_towards() it */
        void mutate_category( const std::string &mut_cat );
        /** Mutates toward one of the given mutations, upgrading or removing conflicts if necessary */
        bool mutate_towards( std::vector<trait_id> muts, int num_tries = INT_MAX );
        /** Mutates toward the entered mutation, upgrading or removing conflicts if necessary */
        bool mutate_towards( const trait_id &mut );
        /** Removes a mutation, downgrading to the previous level if possible */
        void remove_mutation( const trait_id &mut, bool silent = false );
        /** Calculate percentage chances for mutations */
        std::map<trait_id, float> mutation_chances() const;
        /** Returns true if the player has the entered mutation child flag */
        bool has_child_flag( const trait_id &flag ) const;
        /** Removes the mutation's child flag from the player's list */
        void remove_child_flag( const trait_id &flag );
        /** Recalculates mutation_category_level[] values for the player */
        void set_highest_cat_level();
        /** Returns the highest mutation category */
        std::string get_highest_category() const;
        /** Recalculates mutation drench protection for all bodyparts (ignored/good/neutral stats) */
        void drench_mut_calc();
        /** Recursively traverses the mutation's prerequisites and replacements, building up a map */
        void build_mut_dependency_map( const trait_id &mut,
                                       std::unordered_map<trait_id, int> &dependency_map, int distance );

        /**
        * Returns true if this category of mutation is allowed.
        */
        bool is_category_allowed( const std::vector<std::string> &category ) const;
        bool is_category_allowed( const std::string &category ) const;

        bool is_weak_to_water() const;

        /**Check for mutation disallowing the use of an healing item*/
        bool can_use_heal_item( const item &med ) const;

        bool can_install_cbm_on_bp( const std::vector<body_part> &bps ) const;

        /**
         * Returns resistances on a body part provided by mutations
         */
        // TODO: Cache this, it's kinda expensive to compute
        resistances mutation_armor( body_part bp ) const;
        float mutation_armor( body_part bp, damage_type dt ) const;
        float mutation_armor( body_part bp, const damage_unit &du ) const;

        // --------------- Bionic Stuff ---------------
        /** Handles bionic activation effects of the entered bionic, returns if anything activated */
        bool activate_bionic( int b, bool eff_only = false );
        std::vector<bionic_id> get_bionics() const;
        /** Returns amount of Storage CBMs in the corpse **/
        std::pair<int, int> amount_of_storage_bionics() const;
        /** Returns true if the player has the entered bionic id */
        bool has_bionic( const bionic_id &b ) const;
        /** Returns true if the player has the entered bionic id and it is powered on */
        bool has_active_bionic( const bionic_id &b ) const;
        /**Returns true if the player has any bionic*/
        bool has_any_bionic() const;
        /**Returns true if the character can fuel a bionic with the item*/
        bool can_fuel_bionic_with( const item &it ) const;
        /**Return bionic_id of bionics able to use it as fuel*/
        std::vector<bionic_id> get_bionic_fueled_with( const item &it ) const;
        /**Return bionic_id of fueled bionics*/
        std::vector<bionic_id> get_fueled_bionics() const;
        /**Returns bionic_id of first remote fueled bionic found*/
        bionic_id get_remote_fueled_bionic() const;
        /**Return bionic_id of bionic of most fuel efficient bionic*/
        bionic_id get_most_efficient_bionic( const std::vector<bionic_id> &bids ) const;
        /**Return list of available fuel for this bionic*/
        std::vector<itype_id> get_fuel_available( const bionic_id &bio ) const;
        /**Return available space to store specified fuel*/
        int get_fuel_capacity( const itype_id &fuel ) const;
        /**Return total space to store specified fuel*/
        int get_total_fuel_capacity( const itype_id &fuel ) const;
        /**Updates which bionic contain fuel and which is empty*/
        void update_fuel_storage( const itype_id &fuel );
        /**Get stat bonus from bionic*/
        int get_mod_stat_from_bionic( const Character::stat &Stat ) const;
        // route for overmap-scale traveling
        std::vector<tripoint> omt_path;

        /** Handles bionic effects over time of the entered bionic */
        void process_bionic( int b );
        /** Handles bionic deactivation effects of the entered bionic, returns if anything
         *  deactivated */
        bool deactivate_bionic( int b, bool eff_only = false );
        /** Returns the size of my_bionics[] */
        int num_bionics() const;
        /** Returns the bionic at a given index in my_bionics[] */
        bionic &bionic_at_index( int i );
        /** Remove all bionics */
        void clear_bionics();
        int get_used_bionics_slots( body_part bp ) const;
        int get_total_bionics_slots( body_part bp ) const;
        int get_free_bionics_slots( body_part bp ) const;

        /**Has enough anesthetic for surgery*/
        bool has_enough_anesth( const itype *cbm, player &patient );
        /** Handles process of introducing patient into anesthesia during Autodoc operations. Requires anesthesia kits or NOPAIN mutation */
        void introduce_into_anesthesia( const time_duration &duration, player &installer,
                                        bool needs_anesthesia );
        /** Removes a bionic from my_bionics[] */
        void remove_bionic( const bionic_id &b );
        /** Adds a bionic to my_bionics[] */
        void add_bionic( const bionic_id &b );
        /**Calculate skill bonus from tiles in radius*/
        float env_surgery_bonus( int radius );
        /** Calculate skill for (un)installing bionics */
        float bionics_adjusted_skill( const skill_id &most_important_skill,
                                      const skill_id &important_skill,
                                      const skill_id &least_important_skill,
                                      int skill_level = -1 );
        /** Calculate non adjusted skill for (un)installing bionics */
        int bionics_pl_skill( const skill_id &most_important_skill,
                              const skill_id &important_skill,
                              const skill_id &least_important_skill,
                              int skill_level = -1 );
        /**Is the installation possible*/
        bool can_install_bionics( const itype &type, player &installer, bool autodoc = false,
                                  int skill_level = -1 );
        std::map<body_part, int> bionic_installation_issues( const bionic_id &bioid );
        /** Initialize all the values needed to start the operation player_activity */
        bool install_bionics( const itype &type, player &installer, bool autodoc = false,
                              int skill_level = -1 );
        /**Success or failure of installation happens here*/
        void perform_install( bionic_id bid, bionic_id upbid, int difficulty, int success,
                              int pl_skill, const std::string &installer_name,
                              const std::vector<trait_id> &trait_to_rem, const tripoint &patient_pos );
        void bionics_install_failure( const bionic_id &bid, const std::string &installer, int difficulty,
                                      int success, float adjusted_skill, const tripoint &patient_pos );

        /**Is The uninstallation possible*/
        bool can_uninstall_bionic( const bionic_id &b_id, player &installer, bool autodoc = false,
                                   int skill_level = -1 );
        /** Initialize all the values needed to start the operation player_activity */
        bool uninstall_bionic( const bionic_id &b_id, player &installer, bool autodoc = false,
                               int skill_level = -1 );
        /**Succes or failure of removal happens here*/
        void perform_uninstall( bionic_id bid, int difficulty, int success, const units::energy &power_lvl,
                                int pl_skill );
        /**When a player fails the surgery*/
        void bionics_uninstall_failure( int difficulty, int success, float adjusted_skill );

        /**Used by monster to perform surgery*/
        bool uninstall_bionic( const bionic &target_cbm, monster &installer, player &patient,
                               float adjusted_skill );
        /**When a monster fails the surgery*/
        void bionics_uninstall_failure( monster &installer, player &patient, int difficulty, int success,
                                        float adjusted_skill );

        /**Convert fuel to bionic power*/
        bool burn_fuel( int b, bool start = false );
        /**Passively produce power from PERPETUAL fuel*/
        void passive_power_gen( int b );
        /**Find fuel used by remote powered bionic*/
        itype_id find_remote_fuel( bool look_only = false );
        /**Consume fuel used by remote powered bionic, return amount of request unfulfilled (0 if totally successful).*/
        int consume_remote_fuel( int amount );
        void reset_remote_fuel();
        /**Handle heat from exothermic power generation*/
        void heat_emission( int b, int fuel_energy );
        /**Applies modifier to fuel_efficiency and returns the resulting efficiency*/
        float get_effective_efficiency( int b, float fuel_efficiency );

        units::energy get_power_level() const;
        units::energy get_max_power_level() const;
        void mod_power_level( const units::energy &npower );
        void mod_max_power_level( const units::energy &npower_max );
        void set_power_level( const units::energy &npower );
        void set_max_power_level( const units::energy &npower_max );
        bool is_max_power() const;
        bool has_power() const;
        bool has_max_power() const;
        bool enough_power_for( const bionic_id &bid ) const;
        // --------------- Generic Item Stuff ---------------

        struct has_mission_item_filter {
            int mission_id;
            bool operator()( const item &it ) {
                return it.mission_id == mission_id;
            }
        };

        // -2 position is 0 worn index, -3 position is 1 worn index, etc
        static int worn_position_to_index( int position ) {
            return -2 - position;
        }

        // checks to see if an item is worn
        bool is_worn( const item &thing ) const {
            for( const auto &elem : worn ) {
                if( &thing == &elem ) {
                    return true;
                }
            }
            return false;
        }

        /**
         * Asks how to use the item (if it has more than one use_method) and uses it.
         * Returns true if it destroys the item. Consumes charges from the item.
         * Multi-use items are ONLY supported when all use_methods are iuse_actor!
         */
        virtual bool invoke_item( item *, const tripoint &pt );
        /** As above, but with a pre-selected method. Debugmsg if this item doesn't have this method. */
        virtual bool invoke_item( item *, const std::string &, const tripoint &pt );
        /** As above two, but with position equal to current position */
        virtual bool invoke_item( item * );
        virtual bool invoke_item( item *, const std::string & );

        /**
         * Drop, wear, stash or otherwise try to dispose of an item consuming appropriate moves
         * @param obj item to dispose of
         * @param prompt optional message to display in any menu
         * @return whether the item was successfully disposed of
         */
        virtual bool dispose_item( item_location &&obj, const std::string &prompt = std::string() );

        /**
         * Has the item enough charges to invoke its use function?
         * Also checks if UPS from this player is used instead of item charges.
         */
        bool has_enough_charges( const item &it, bool show_msg ) const;

        /** Consume charges of a tool or comestible item, potentially destroying it in the process
         *  @param used item consuming the charges
         *  @param qty number of charges to consume which must be non-zero
         *  @return true if item was destroyed */
        bool consume_charges( item &used, int qty );

        /**
         * Calculate (but do not deduct) the number of moves required when handling (e.g. storing, drawing etc.) an item
         * @param it Item to calculate handling cost for
         * @param penalties Whether item volume and temporary effects (e.g. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         * @return cost in moves ranging from 0 to MAX_HANDLING_COST
         */
        int item_handling_cost( const item &it, bool penalties = true,
                                int base_cost = INVENTORY_HANDLING_PENALTY ) const;

        /**
         * Calculate (but do not deduct) the number of moves required when storing an item in a container
         * @param it Item to calculate storage cost for
         * @param container Container to store item in
         * @param penalties Whether item volume and temporary effects (e.g. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         * @return cost in moves ranging from 0 to MAX_HANDLING_COST
         */
        int item_store_cost( const item &it, const item &container, bool penalties = true,
                             int base_cost = INVENTORY_HANDLING_PENALTY ) const;

        /** Calculate (but do not deduct) the number of moves required to wear an item */
        int item_wear_cost( const item &it ) const;

        /** Wear item; returns nullopt on fail, or pointer to newly worn item on success.
         * If interactive is false, don't alert the player or drain moves on completion.
         */
        cata::optional<std::list<item>::iterator>
        wear_item( const item &to_wear, bool interactive = true );

        /** Returns the amount of item `type' that is currently worn */
        int  amount_worn( const itype_id &id ) const;

        /** Returns nearby items which match the provided predicate */
        std::vector<item_location> nearby( const std::function<bool( const item *, const item * )> &func,
                                           int radius = 1 ) const;

        /**
         * Similar to @ref remove_items_with, but considers only worn items and not their
         * content (@ref item::contents is not checked).
         * If the filter function returns true, the item is removed.
         */
        std::list<item> remove_worn_items_with( std::function<bool( item & )> filter );

        /** Return the item pointer of the item with given invlet, return nullptr if
         * the player does not have such an item with that invlet. Don't use this on npcs.
         * Only use the invlet in the user interface, otherwise always use the item position. */
        item *invlet_to_item( int invlet );

        // Returns the item with a given inventory position.
        item &i_at( int position );
        const item &i_at( int position ) const;
        /**
         * Returns the item position (suitable for @ref i_at or similar) of a
         * specific item. Returns INT_MIN if the item is not found.
         * Note that this may lose some information, for example the returned position is the
         * same when the given item points to the container and when it points to the item inside
         * the container. All items that are part of the same stack have the same item position.
         */
        int get_item_position( const item *it ) const;

        /**
         * Returns a reference to the item which will be used to make attacks.
         * At the moment it's always @ref weapon or a reference to a null item.
         */
        /*@{*/
        const item &used_weapon() const;
        item &used_weapon();
        /*@}*/

        /**
         * Try to find a container/s on character containing ammo of type it.typeId() and
         * add charges until the container is full.
         * @param unloading Do not try to add to a container when the item was intentionally unloaded.
         * @return Remaining charges which could not be stored in a container.
         */
        int i_add_to_container( const item &it, bool unloading );
        item &i_add( item it, bool should_stack = true );

        /**
         * Try to pour the given liquid into the given container/vehicle. The transferred charges are
         * removed from the liquid item. Check the charges of afterwards to see if anything has
         * been transferred at all.
         * The functions do not consume any move points.
         * @return Whether anything has been moved at all. `false` indicates the transfer is not
         * possible at all. `true` indicates at least some of the liquid has been moved.
         */
        /**@{*/
        bool pour_into( item &container, item &liquid );
        bool pour_into( vehicle &veh, item &liquid );
        /**@}*/

        /**
         * Remove a specific item from player possession. The item is compared
         * by pointer. Contents of the item are removed as well.
         * @param pos The item position of the item to be removed. The item *must*
         * exists, use @ref has_item to check this.
         * @return A copy of the removed item.
         */
        item i_rem( int pos );
        /**
         * Remove a specific item from player possession. The item is compared
         * by pointer. Contents of the item are removed as well.
         * @param it A pointer to the item to be removed. The item *must* exists
         * in the players possession (one can use @ref has_item to check for this).
         * @return A copy of the removed item.
         */
        item i_rem( const item *it );
        void i_rem_keep_contents( int idx );
        /** Sets invlet and adds to inventory if possible, drops otherwise, returns true if either succeeded.
         *  An optional qty can be provided (and will perform better than separate calls). */
        bool i_add_or_drop( item &it, int qty = 1 );

        /** Only use for UI things. Returns all invlets that are currently used in
         * the player inventory, the weapon slot and the worn items. */
        std::bitset<std::numeric_limits<char>::max()> allocated_invlets() const;

        /**
         * Whether the player carries an active item of the given item type.
         */
        bool has_active_item( const itype_id &id ) const;
        item remove_weapon();
        void remove_mission_items( int mission_id );

        /**
         * Returns the items that are ammo and have the matching ammo type.
         */
        std::vector<const item *> get_ammo( const ammotype &at ) const;

        /**
         * Searches for ammo or magazines that can be used to reload obj
         * @param obj item to be reloaded. By design any currently loaded ammunition or magazine is ignored
         * @param empty whether empty magazines should be considered as possible ammo
         * @param radius adjacent map/vehicle tiles to search. 0 for only player tile, -1 for only inventory
         */
        std::vector<item_location> find_ammo( const item &obj, bool empty = true, int radius = 1 ) const;

        /**
         * Searches for weapons and magazines that can be reloaded.
         */
        std::vector<item_location> find_reloadables();
        /**
         * Counts ammo and UPS charges (lower of) for a given gun on the character.
         */
        int ammo_count_for( const item &gun );

        /** Maximum thrown range with a given item, taking all active effects into account. */
        int throw_range( const item & ) const;
        /** Dispersion of a thrown item, against a given target, taking into account whether or not the throw was blind. */
        int throwing_dispersion( const item &to_throw, Creature *critter = nullptr,
                                 bool is_blind_throw = false ) const;
        /** How much dispersion does one point of target's dodge add when throwing at said target? */
        int throw_dispersion_per_dodge( bool add_encumbrance = true ) const;

        /** True if unarmed or wielding a weapon with the UNARMED_WEAPON flag */
        bool unarmed_attack() const;
        /// Checks for items, tools, and vehicles with the Lifting quality near the character
        /// returning the highest quality in range.
        int best_nearby_lifting_assist() const;

        /// Alternate version if you need to specify a different orign point for nearby vehicle sources of lifting
        /// used for operations on distant objects (e.g. vehicle installation/uninstallation)
        int best_nearby_lifting_assist( const tripoint &world_pos ) const;

        // Inventory + weapon + worn (for death, etc)
        std::vector<item *> inv_dump();

        units::mass weight_carried() const;
        units::volume volume_carried() const;

        /// Sometimes we need to calculate hypothetical volume or weight.  This
        /// struct offers two possible tweaks: a collection of items and
        /// coutnts to remove, or an entire replacement inventory.
        struct item_tweaks {
            item_tweaks() = default;
            item_tweaks( const std::map<const item *, int> &w ) :
                without_items( std::cref( w ) )
            {}
            item_tweaks( const inventory &r ) :
                replace_inv( std::cref( r ) )
            {}
            const cata::optional<std::reference_wrapper<const std::map<const item *, int>>> without_items;
            const cata::optional<std::reference_wrapper<const inventory>> replace_inv;
        };

        units::mass weight_carried_with_tweaks( const item_tweaks & ) const;
        units::volume volume_carried_with_tweaks( const item_tweaks & ) const;
        units::mass weight_capacity() const override;
        units::volume volume_capacity() const;
        units::volume volume_capacity_reduced_by(
            const units::volume &mod,
            const std::map<const item *, int> &without_items = {} ) const;

        bool can_pickVolume( const item &it, bool safe = false ) const;
        bool can_pickWeight( const item &it, bool safe = true ) const;
        /**
         * Checks if character stats and skills meet minimum requirements for the item.
         * Prints an appropriate message if requirements not met.
         * @param it Item we are checking
         * @param context optionally override effective item when checking contextual skills
         */
        bool can_use( const item &it, const item &context = item() ) const;
        /**
         * Check character capable of wearing an item.
         * @param it Thing to be worn
         * @param with_equip_change If true returns if it could be worn if things were taken off
         */
        ret_val<bool> can_wear( const item &it, bool with_equip_change = false ) const;
        /**
         * Returns true if the character is wielding something.
         * Note: this item may not actually be used to attack.
         */
        bool is_armed() const;

        /**
         * Removes currently wielded item (if any) and replaces it with the target item.
         * @param target replacement item to wield or null item to remove existing weapon without replacing it
         * @return whether both removal and replacement were successful (they are performed atomically)
         */
        virtual bool wield( item &target ) = 0;
        /**
         * Check player capable of unwielding an item.
         * @param it Thing to be unwielded
         */
        ret_val<bool> can_unwield( const item &it ) const;

        void drop_invalid_inventory();
        /** Returns all items that must be taken off before taking off this item */
        std::list<item *> get_dependent_worn_items( const item &it );
        /** Drops an item to the specified location */
        void drop( item_location loc, const tripoint &where );
        virtual void drop( const drop_locations &what, const tripoint &target, bool stash = false );

        virtual bool has_artifact_with( art_effect_passive effect ) const;

        bool is_wielding( const item &target ) const;

        bool covered_with_flag( const std::string &flag, const body_part_set &parts ) const;
        bool is_waterproof( const body_part_set &parts ) const;
        // Carried items may leak radiation or chemicals
        int leak_level( const std::string &flag ) const;

        // --------------- Clothing Stuff ---------------
        /** Returns true if the player is wearing the item. */
        bool is_wearing( const itype_id &it ) const;
        /** Returns true if the player is wearing the item on the given body_part. */
        bool is_wearing_on_bp( const itype_id &it, body_part bp ) const;
        /** Returns true if the player is wearing an item with the given flag. */
        bool worn_with_flag( const std::string &flag, body_part bp = num_bp ) const;
        /** Returns the first worn item with a given flag. */
        item item_worn_with_flag( const std::string &flag, body_part bp = num_bp ) const;

        // drawing related stuff
        /**
         * Returns a list of the IDs of overlays on this character,
         * sorted from "lowest" to "highest".
         *
         * Only required for rendering.
         */
        std::vector<std::string> get_overlay_ids() const;

        // --------------- Skill Stuff ---------------
        int get_skill_level( const skill_id &ident ) const;
        int get_skill_level( const skill_id &ident, const item &context ) const;

        const SkillLevelMap &get_all_skills() const;
        SkillLevel &get_skill_level_object( const skill_id &ident );
        const SkillLevel &get_skill_level_object( const skill_id &ident ) const;

        void set_skill_level( const skill_id &ident, int level );
        void mod_skill_level( const skill_id &ident, int delta );
        /** Checks whether the character's skills meet the required */
        bool meets_skill_requirements( const std::map<skill_id, int> &req,
                                       const item &context = item() ) const;
        /** Checks whether the character's skills meet the required */
        bool meets_skill_requirements( const construction &con ) const;
        /** Checks whether the character's stats meets the stats required by the item */
        bool meets_stat_requirements( const item &it ) const;
        /** Checks whether the character meets overall requirements to be able to use the item */
        bool meets_requirements( const item &it, const item &context = item() ) const;
        /** Returns a string of missed requirements (both stats and skills) */
        std::string enumerate_unmet_requirements( const item &it, const item &context = item() ) const;

        /** Returns the player's skill rust rate */
        int rust_rate() const;

        // Mental skills and stats
        /** Returns the player's reading speed */
        int read_speed( bool return_stat_effect = true ) const;

        // --------------- Other Stuff ---------------

        /** return the calendar::turn the character expired */
        time_point get_time_died() const {
            return time_died;
        }
        /** set the turn the turn the character died if not already done */
        void set_time_died( const time_point &time ) {
            if( time_died != calendar::before_time_starts ) {
                time_died = time;
            }
        }
        // magic mod
        known_magic magic;

        void make_bleed( body_part bp, time_duration duration, int intensity = 1,
                         bool permanent = false,
                         bool force = false, bool defferred = false );

        /** Calls Creature::normalize()
         *  nulls out the player's weapon
         *  Should only be called through player::normalize(), not on it's own!
         */
        void normalize() override;
        void die( Creature *nkiller ) override;

        std::string get_name() const override;

        std::vector<std::string> get_grammatical_genders() const override;

        /**
         * It is supposed to hide the query_yn to simplify player vs. npc code.
         */
        template<typename ...Args>
        bool query_yn( const char *const msg, Args &&... args ) const {
            return query_yn( string_format( msg, std::forward<Args>( args ) ... ) );
        }
        virtual bool query_yn( const std::string &msg ) const = 0;

        bool is_immune_field( const field_type_id &fid ) const override;
        /** Returns true is the player is protected from electric shocks */
        bool is_elec_immune() const override;
        /** Returns true if the player is immune to this kind of effect */
        bool is_immune_effect( const efftype_id & ) const override;
        /** Returns true if the player is immune to this kind of damage */
        bool is_immune_damage( damage_type ) const override;
        /** Returns true if the player is protected from radiation */
        bool is_rad_immune() const;
        /** Returns true if the player is immune to throws */
        bool is_throw_immune() const;

        /** Returns true if the player has some form of night vision */
        bool has_nv();

        /**
         * Returns >0 if character is sitting/lying and relatively inactive.
         * 1 represents sleep on comfortable bed, so anything above that should be rare.
         */
        float rest_quality() const;
        /**
         * Average hit points healed per turn.
         */
        float healing_rate( float at_rest_quality ) const;
        /**
         * Average hit points healed per turn from healing effects.
         */
        float healing_rate_medicine( float at_rest_quality, body_part bp ) const;

        /**
         * Goes over all mutations, gets min and max of a value with given name
         * @return min( 0, lowest ) + max( 0, highest )
         */
        float mutation_value( const std::string &val ) const;

        /**
         * Goes over all mutations, returning the sum of the social modifiers
         */
        social_modifiers get_mutation_social_mods() const;

        /** Color's character's tile's background */
        nc_color symbol_color() const override;

        std::string extended_description() const override;

        // In newcharacter.cpp
        void empty_skills();
        /** Returns a random name from NAMES_* */
        void pick_name( bool bUseDefault = false );
        /** Get the idents of all base traits. */
        std::vector<trait_id> get_base_traits() const;
        /** Get the idents of all traits/mutations. */
        std::vector<trait_id> get_mutations( bool include_hidden = true ) const;
        const std::bitset<NUM_VISION_MODES> &get_vision_modes() const {
            return vision_mode_cache;
        }
        /** Empties the trait and mutations lists */
        void clear_mutations();
        /**
         * Adds mandatory scenario and profession traits unless you already have them
         * And if you do already have them, refunds the points for the trait
         */
        void add_traits();
        void add_traits( points_left &points );
        /** Returns true if the player has crossed a mutation threshold
         *  Player can only cross one mutation threshold.
         */
        bool crossed_threshold() const;

        // --------------- Values ---------------
        std::string name;
        bool male;

        std::list<item> worn;
        std::array<int, num_hp_parts> hp_cur, hp_max, damage_bandaged, damage_disinfected;
        bool nv_cached;
        // Means player sit inside vehicle on the tile he is now
        bool in_vehicle;
        bool hauling;

        player_activity stashed_outbounds_activity;
        player_activity stashed_outbounds_backlog;
        player_activity activity;
        std::list<player_activity> backlog;
        cata::optional<tripoint> destination_point;
        inventory inv;
        itype_id last_item;
        item weapon;

        int scent;
        pimpl<bionic_collection> my_bionics;
        character_martial_arts martial_arts_data;

        stomach_contents stomach;
        std::list<consumption_event> consumption_history;

        int oxygen;
        int tank_plut;
        int reactor_plut;
        int slow_rad;

        int focus_pool;
        int cash;
        std::set<character_id> follower_ids;
        weak_ptr_fast<Creature> last_target;
        cata::optional<tripoint> last_target_pos;
        // Save favorite ammo location
        item_location ammo_location;
        std::set<tripoint> camps;
        /* crafting inventory cached time */
        time_point cached_time;

        std::vector <addiction> addictions;
        /** Adds an addiction to the player */
        void add_addiction( add_type type, int strength );
        /** Removes an addition from the player */
        void rem_addiction( add_type type );
        /** Returns true if the player has an addiction of the specified type */
        bool has_addiction( add_type type ) const;
        /** Returns the intensity of the specified addiction */
        int  addiction_level( add_type type ) const;

        shared_ptr_fast<monster> mounted_creature;
        // for loading NPC mounts
        int mounted_creature_id;
        // for vehicle work
        int activity_vehicle_part_index = -1;

        // Hauling items on the ground
        void start_hauling();
        void stop_hauling();
        bool is_hauling() const;

        // Has a weapon, inventory item or worn item with flag
        bool has_item_with_flag( const std::string &flag, bool need_charges = false ) const;
        /**
         * All items that have the given flag (@ref item::has_flag).
         */
        std::vector<const item *> all_items_with_flag( const std::string &flag ) const;

        bool has_charges( const itype_id &it, int quantity,
                          const std::function<bool( const item & )> &filter = return_true<item> ) const;

        // has_amount works ONLY for quantity.
        // has_charges works ONLY for charges.
        std::list<item> use_amount( itype_id it, int quantity,
                                    const std::function<bool( const item & )> &filter = return_true<item> );
        // Uses up charges
        bool use_charges_if_avail( const itype_id &it, int quantity );

        // Uses up charges
        std::list<item> use_charges( const itype_id &what, int qty,
                                     const std::function<bool( const item & )> &filter = return_true<item> );

        bool has_fire( int quantity ) const;
        void use_fire( int quantity );
        void assign_stashed_activity();
        bool check_outbounds_activity( const player_activity &act, bool check_only = false );
        /** Legacy activity assignment, does not work for any activites using
         * the new activity_actor class and may cause issues with resuming.
         * TODO: delete this once migration of activites to the activity_actor system is complete
         */
        void assign_activity( const activity_id &type, int moves = calendar::INDEFINITELY_LONG,
                              int index = -1, int pos = INT_MIN,
                              const std::string &name = "" );
        /** Assigns activity to player, possibly resuming old activity if it's similar enough. */
        void assign_activity( const player_activity &act, bool allow_resume = true );
        /** Check if player currently has a given activity */
        bool has_activity( const activity_id &type ) const;
        /** Check if player currently has any of the given activities */
        bool has_activity( const std::vector<activity_id> &types ) const;
        void resume_backlog_activity();
        void cancel_activity();
        void cancel_stashed_activity();
        player_activity get_stashed_activity() const;
        void set_stashed_activity( const player_activity &act,
                                   const player_activity &act_back = player_activity() );
        bool has_stashed_activity() const;
        void initialize_stomach_contents();

        /** Stable base metabolic rate due to traits */
        float metabolic_rate_base() const;
        /** Current metabolic rate due to traits, hunger, speed, etc. */
        float metabolic_rate() const;
        // Your mass + "kg/lbs"
        std::string get_weight_string() const;
        // gets the max value healthy you can be
        int get_max_healthy() const;
        // calculates the BMI
        float bmi() const;
        // returns amount of calories burned in a day given various metabolic factors
        int bmr() const;
        // Reset age and height to defaults for consistent test results
        void reset_chargen_attributes();
        // age in years, determined at character creation
        int base_age() const;
        void set_base_age( int age );
        void mod_base_age( int mod );
        // age in years
        int age() const;
        std::string age_string() const;
        // returns the height in cm
        int base_height() const;
        void set_base_height( int height );
        void mod_base_height( int mod );
        std::string height_string() const;
        // returns the height of the player character in cm
        int height() const;
        // returns bodyweight of the Character
        units::mass bodyweight() const;
        // returns total weight of installed bionics
        units::mass bionics_weight() const;

        /** Returns overall bashing resistance for the body_part */
        int get_armor_bash( bodypart_id bp ) const override;
        /** Returns overall cutting resistance for the body_part */
        int get_armor_cut( bodypart_id bp ) const override;
        /** Returns bashing resistance from the creature and armor only */
        int get_armor_bash_base( bodypart_id bp ) const override;
        /** Returns cutting resistance from the creature and armor only */
        int get_armor_cut_base( bodypart_id bp ) const override;
        /** Returns overall env_resist on a body_part */
        int get_env_resist( bodypart_id bp ) const override;
        /** Returns overall acid resistance for the body part */
        int get_armor_acid( bodypart_id bp ) const;
        /** Returns overall resistance to given type on the bod part */
        int get_armor_type( damage_type dt, bodypart_id bp ) const override;

        int get_stim() const;
        void set_stim( int new_stim );
        void mod_stim( int mod );

        int get_rad() const;
        void set_rad( int new_rad );
        void mod_rad( int mod );

        int get_stamina() const;
        int get_stamina_max() const;
        void set_stamina( int new_stamina );
        void mod_stamina( int mod );
        void burn_move_stamina( int moves );
        float stamina_move_cost_modifier() const;
        /** Regenerates stamina */
        void update_stamina( int turns );

    protected:
        void on_damage_of_type( int adjusted_damage, damage_type type, body_part bp ) override;
    public:
        /** Called when an item is worn */
        void on_item_wear( const item &it );
        /** Called when an item is taken off */
        void on_item_takeoff( const item &it );
        /** Called when an item is washed */
        void on_worn_item_washed( const item &it );
        /** Called when effect intensity has been changed */
        void on_effect_int_change( const efftype_id &eid, int intensity, body_part bp = num_bp ) override;
        /** Called when a mutation is gained */
        void on_mutation_gain( const trait_id &mid );
        /** Called when a mutation is lost */
        void on_mutation_loss( const trait_id &mid );
        /** Called when a stat is changed */
        void on_stat_change( const std::string &stat, int value ) override;
        /** Returns an unoccupied, safe adjacent point. If none exists, returns player position. */
        tripoint adjacent_tile() const;
        /** Returns true if the player has a trait which cancels the entered trait */
        bool has_opposite_trait( const trait_id &flag ) const;
        /** Removes "sleep" and "lying_down" */
        void wake_up();
        // how loud a character can shout. based on mutations and clothing
        int get_shout_volume() const;
        // shouts a message
        void shout( std::string msg = "", bool order = false );
        /** Handles Character vomiting effects */
        void vomit();
        // adds total healing to the bodypart. this is only a counter.
        void healed_bp( int bp, int amount );

        // the amount healed per bodypart per day
        std::array<int, num_hp_parts> healed_total;

        std::map<std::string, int> mutation_category_level;

        int adjust_for_focus( int amount ) const;
        void update_type_of_scent( bool init = false );
        void update_type_of_scent( const trait_id &mut, bool gain = true );
        void set_type_of_scent( const scenttype_id &id );
        scenttype_id get_type_of_scent() const;
        /**restore scent after masked_scent effect run out or is removed by water*/
        void restore_scent();
        /** Modifies intensity of painkillers  */
        void mod_painkiller( int npkill );
        /** Sets intensity of painkillers  */
        void set_painkiller( int npkill );
        /** Returns intensity of painkillers  */
        int get_painkiller() const;
        void react_to_felt_pain( int intensity );

        void spores();
        void blossoms();

        /** Handles rooting effects */
        void rooted_message() const;
        void rooted();

        /** Adds "sleep" to the player */
        void fall_asleep();
        void fall_asleep( const time_duration &duration );
        /** Checks to see if the player is using floor items to keep warm, and return the name of one such item if so */
        std::string is_snuggling() const;

        /** Prompts user about crushing item at item_location loc, for harvesting of frozen liquids
        * @param loc Location for item to crush */
        bool crush_frozen_liquid( item_location loc );

        float power_rating() const override;
        float speed_rating() const override;

        /** Returns the item in the player's inventory with the highest of the specified quality*/
        item &item_with_best_of_quality( const quality_id &qid );
        /**
         * Check whether the this player can see the other creature with infrared. This implies
         * this player can see infrared and the target is visible with infrared (is warm).
         * And of course a line of sight exists.
        */
        bool sees_with_infrared( const Creature &critter ) const;
        // Put corpse+inventory on map at the place where this is.
        void place_corpse();
        // Put corpse+inventory on defined om tile
        void place_corpse( const tripoint &om_target );

        /** Returns the player's modified base movement cost */
        int  run_cost( int base_cost, bool diag = false ) const;
        const pathfinding_settings &get_pathfinding_settings() const override;
        std::set<tripoint> get_path_avoid() const override;
        /**
         * Get all hostile creatures currently visible to this player.
         */
        std::vector<Creature *> get_hostile_creatures( int range ) const;

        /**
         * Returns all creatures that this player can see and that are in the given
         * range. This player object itself is never included.
         * The player character (g->u) is checked and might be included (if applicable).
         * @param range The maximal distance (@ref rl_dist), creatures at this distance or less
         * are included.
         */
        std::vector<Creature *> get_visible_creatures( int range ) const;
        /** Returns an enumeration of visible mutations with colors */
        std::string visible_mutations( int visibility_cap ) const;
        player_activity get_destination_activity() const;
        void set_destination_activity( const player_activity &new_destination_activity );
        void clear_destination_activity();
        /** Returns warmth provided by armor, etc. */
        int warmth( body_part bp ) const;
        /** Returns warmth provided by an armor's bonus, like hoods, pockets, etc. */
        int bonus_item_warmth( body_part bp ) const;
        /** Can the player lie down and cover self with blankets etc. **/
        bool can_use_floor_warmth() const;
        /**
         * Warmth from terrain, furniture, vehicle furniture and traps.
         * Can be negative.
         **/
        static int floor_bedding_warmth( const tripoint &pos );
        /** Warmth from clothing on the floor **/
        static int floor_item_warmth( const tripoint &pos );
        /** Final warmth from the floor **/
        int floor_warmth( const tripoint &pos ) const;

        /** Correction factor of the body temperature due to traits and mutations **/
        int bodytemp_modifier_traits( bool overheated ) const;
        /** Correction factor of the body temperature due to traits and mutations for player lying on the floor **/
        int bodytemp_modifier_traits_floor() const;
        /** Value of the body temperature corrected by climate control **/
        int temp_corrected_by_climate_control( int temperature ) const;

        bool in_sleep_state() const override {
            return Creature::in_sleep_state() || activity.id() == "ACT_TRY_SLEEP";
        }

        /** Set vitamin deficiency/excess disease states dependent upon current vitamin levels */
        void update_vitamins( const vitamin_id &vit );
        /**
         * Check current level of a vitamin
         *
         * Accesses level of a given vitamin.  If the vitamin_id specified does not
         * exist then this function simply returns 0.
         *
         * @param vit ID of vitamin to check level for.
         * @returns current level for specified vitamin
         */
        int vitamin_get( const vitamin_id &vit ) const;
        /**
         * Sets level of a vitamin or returns false if id given in vit does not exist
         *
         * @note status effects are still set for deficiency/excess
         *
         * @param[in] vit ID of vitamin to adjust quantity for
         * @param[in] qty Quantity to set level to
         * @returns false if given vitamin_id does not exist, otherwise true
         */
        bool vitamin_set( const vitamin_id &vit, int qty );
        /**
          * Add or subtract vitamins from character storage pools
         * @param vit ID of vitamin to modify
         * @param qty amount by which to adjust vitamin (negative values are permitted)
         * @param capped if true prevent vitamins which can accumulate in excess from doing so
         * @return adjusted level for the vitamin or zero if vitamin does not exist
         */
        int vitamin_mod( const vitamin_id &vit, int qty, bool capped = true );
        void vitamins_mod( const std::map<vitamin_id, int> &, bool capped = true );
        /** Get vitamin usage rate (minutes per unit) accounting for bionics, mutations and effects */
        time_duration vitamin_rate( const vitamin_id &vit ) const;

        /** Handles the nutrition value for a comestible **/
        int nutrition_for( const item &comest ) const;
        /** Can the food be [theoretically] eaten no matter the consequen
        ces? */
        ret_val<edible_rating> can_eat( const item &food ) const;
        /**
         * Same as @ref can_eat, but takes consequences into account.
         * Asks about them if @param interactive is true, refuses otherwise.
         */
        ret_val<edible_rating> will_eat( const item &food, bool interactive = false ) const;
        /** Determine character's capability of recharging their CBMs. */
        bool can_feed_reactor_with( const item &it ) const;
        bool can_feed_furnace_with( const item &it ) const;
        rechargeable_cbm get_cbm_rechargeable_with( const item &it ) const;
        int get_acquirable_energy( const item &it, rechargeable_cbm cbm ) const;
        int get_acquirable_energy( const item &it ) const;

        /**
        * Recharge CBMs whenever possible.
        * @return true when recharging was successful.
        */
        bool feed_reactor_with( item &it );
        bool feed_furnace_with( item &it );
        bool fuel_bionic_with( item &it );
        /** Used to apply stimulation modifications from food and medication **/
        void modify_stimulation( const islot_comestible &comest );
        /** Used to apply fatigue modifications from food and medication **/
        void modify_fatigue( const islot_comestible &comest );
        /** Used to apply radiation from food and medication **/
        void modify_radiation( const islot_comestible &comest );
        /** Used to apply addiction modifications from food and medication **/
        void modify_addiction( const islot_comestible &comest );
        /** Used to apply health modifications from food and medication **/
        void modify_health( const islot_comestible &comest );
        /** Handles the effects of consuming an item */
        bool consume_effects( item &food );
        /** Check character's capability of consumption overall */
        bool can_consume( const item &it ) const;
        /** True if the character has enough skill (in cooking or survival) to estimate time to rot */
        bool can_estimate_rot() const;
        /** Check whether character can consume this very item */
        bool can_consume_as_is( const item &it ) const;
        bool can_consume_for_bionic( const item &it ) const;
        /**
         * Returns a reference to the item itself (if it's consumable),
         * the first of its contents (if it's consumable) or null item otherwise.
         * WARNING: consumable does not necessarily guarantee the comestible type.
         */
        item &get_consumable_from( item &it ) const;

        hint_rating rate_action_eat( const item &it ) const;

        /** Get calorie & vitamin contents for a comestible, taking into
         * account character traits */
        /** Get range of possible nutrient content, for a particular recipe,
         * depending on choice of ingredients */
        std::pair<nutrients, nutrients> compute_nutrient_range(
            const item &, const recipe_id &,
            const cata::flat_set<std::string> &extra_flags = {} ) const;
        /** Same, but across arbitrary recipes */
        std::pair<nutrients, nutrients> compute_nutrient_range(
            const itype_id &, const cata::flat_set<std::string> &extra_flags = {} ) const;
        /** Returns allergy type or MORALE_NULL if not allergic for this character */
        morale_type allergy_type( const item &food ) const;
        nutrients compute_effective_nutrients( const item & ) const;
        /** Returns true if the character is wearing something on the entered body_part */
        bool wearing_something_on( body_part bp ) const;
        /** Returns true if the character is wearing something occupying the helmet slot */
        bool is_wearing_helmet() const;
        /** Returns the total encumbrance of all SKINTIGHT and HELMET_COMPAT items coveringi
         *  the head */
        int head_cloth_encumbrance() const;
        /** Same as footwear factor, but for arms */
        double armwear_factor() const;
        /** Returns 1 if the player is wearing an item of that count on one foot, 2 if on both,
         *  and zero if on neither */
        int shoe_type_count( const itype_id &it ) const;
        /** Returns 1 if the player is wearing something on both feet, .5 if on one,
         *  and 0 if on neither */
        double footwear_factor() const;
        /** Returns true if the player is wearing something on their feet that is not SKINTIGHT */
        bool is_wearing_shoes( const side &which_side = side::BOTH ) const;

        /** Swap side on which item is worn; returns false on fail. If interactive is false, don't alert player or drain moves */
        bool change_side( item &it, bool interactive = true );
        bool change_side( item_location &loc, bool interactive = true );

        /** Used to determine player feedback on item use for the inventory code.
         *  rates usability lower for non-tools (books, etc.) */
        hint_rating rate_action_change_side( const item &it ) const;

        bool get_check_encumbrance() {
            return check_encumbrance;
        }
        void set_check_encumbrance( bool new_check ) {
            check_encumbrance = new_check;
        }
        /** Ticks down morale counters and removes them */
        void update_morale();
        /** Ensures persistent morale effects are up-to-date */
        void apply_persistent_morale();
        /** Used to apply morale modifications from food and medication **/
        void modify_morale( item &food, int nutr = 0 );
        // Modified by traits, &c
        int get_morale_level() const;
        void add_morale( const morale_type &type, int bonus, int max_bonus = 0,
                         const time_duration &duration = 1_hours,
                         const time_duration &decay_start = 30_minutes, bool capped = false,
                         const itype *item_type = nullptr );
        int has_morale( const morale_type &type ) const;
        void rem_morale( const morale_type &type, const itype *item_type = nullptr );
        void clear_morale();
        bool has_morale_to_read() const;
        bool has_morale_to_craft() const;
        const inventory &crafting_inventory( bool clear_path );
        const inventory &crafting_inventory( const tripoint &src_pos = tripoint_zero,
                                             int radius = PICKUP_RANGE, bool clear_path = true );
        void invalidate_crafting_inventory();

        /** Checks permanent morale for consistency and recovers it when an inconsistency is found. */
        void check_and_recover_morale();

        /** Handles the enjoyability value for a comestible. First value is enjoyability, second is cap. **/
        std::pair<int, int> fun_for( const item &comest ) const;

        /** Handles a large number of timers decrementing and other randomized effects */
        void suffer();
        /** Handles mitigation and application of radiation */
        bool irradiate( float rads, bool bypass = false );
        /** Handles the chance for broken limbs to spontaneously heal to 1 HP */
        void mend( int rate_multiplier );

        /** Creates an auditory hallucination */
        void sound_hallu();

        /** Drenches the player with water, saturation is the percent gotten wet */
        void drench( int saturation, const body_part_set &flags, bool ignore_waterproof );
        /** Recalculates morale penalty/bonus from wetness based on mutations, equipment and temperature */
        void apply_wetness_morale( int temperature );
        std::vector<std::string> short_description_parts() const;
        std::string short_description() const;
        int print_info( const catacurses::window &w, int vStart, int vLines, int column ) const override;
        // Checks whether a player can hear a sound at a given volume and location.
        bool can_hear( const tripoint &source, int volume ) const;
        // Returns a multiplier indicating the keenness of a player's hearing.
        float hearing_ability() const;

        using trap_map = std::map<tripoint, std::string>;
        bool knows_trap( const tripoint &pos ) const;
        void add_known_trap( const tripoint &pos, const trap &t );
        /** Define color for displaying the body temperature */
        nc_color bodytemp_color( int bp ) const;

        // see Creature::sees
        bool sees( const tripoint &t, bool is_player = false, int range_mod = 0 ) const override;
        // see Creature::sees
        bool sees( const Creature &critter ) const override;
        Attitude attitude_to( const Creature &other ) const override;

        int get_hp( hp_part bp ) const override;
        int get_hp() const override;
        int get_hp_max( hp_part bp ) const override;
        int get_hp_max() const override;
        bool has_weapon() const override;
        void shift_destination( const point &shift );
        // Auto move methods
        void set_destination( const std::vector<tripoint> &route,
                              const player_activity &new_destination_activity = player_activity() );
        void clear_destination();
        bool has_distant_destination() const;

        // true if the player is auto moving, or if the player is going to finish
        // auto moving but the destination is not yet reset, such as in avatar_action::move
        bool is_auto_moving() const;
        // true if there are further moves in the auto move route
        bool has_destination() const;
        // true if player has destination activity AND is standing on destination tile
        bool has_destination_activity() const;
        // starts destination activity and cleans up to ensure it is called only once
        void start_destination_activity();
        std::vector<tripoint> &get_auto_move_route();
        action_id get_next_auto_move_direction();
        bool defer_move( const tripoint &next );

    protected:
        Character();
        Character( Character && );
        Character &operator=( Character && );
        struct trait_data {
            /** Whether the mutation is activated. */
            bool powered = false;
            /** Key to select the mutation in the UI. */
            char key = ' ';
            /**
             * Time (in turns) until the mutation increase hunger/thirst/fatigue according
             * to its cost (@ref mutation_branch::cost). When those costs have been paid, this
             * is reset to @ref mutation_branch::cooldown.
             */
            int charge = 0;
            void serialize( JsonOut &json ) const;
            void deserialize( JsonIn &jsin );
        };

        // The player's position on the local map.
        tripoint position;

        /** Bonuses to stats, calculated each turn */
        int str_bonus;
        int dex_bonus;
        int per_bonus;
        int int_bonus;

        /** How healthy the character is. */
        int healthy;
        int healthy_mod;

        /** age in years at character creation */
        int init_age = 25;
        /**height at character creation*/
        int init_height = 175;
        /** Size class of character. */
        m_size size_class = MS_MEDIUM;

        trap_map known_traps;
        std::array<encumbrance_data, num_bp> encumbrance_cache;
        mutable std::map<std::string, double> cached_info;
        bool bio_soporific_powered_at_last_sleep_check;
        /** last time we checked for sleep */
        time_point last_sleep_check = calendar::turn_zero;
        /** warnings from a faction about bad behavior */
        std::map<faction_id, std::pair<int, time_point>> warning_record;
        /**
         * Traits / mutations of the character. Key is the mutation id (it's also a valid
         * key into @ref mutation_data), the value describes the status of the mutation.
         * If there is not entry for a mutation, the character does not have it. If the map
         * contains the entry, the character has the mutation.
         */
        std::unordered_map<trait_id, trait_data> my_mutations;
        /**
         * Contains mutation ids of the base traits.
         */
        std::unordered_set<trait_id> my_traits;
        /**
         * Pointers to mutation branches in @ref my_mutations.
         */
        std::vector<const mutation_branch *> cached_mutations;

        void store( JsonOut &json ) const;
        void load( const JsonObject &data );

        // --------------- Values ---------------
        pimpl<SkillLevelMap> _skills;

        // Cached vision values.
        std::bitset<NUM_VISION_MODES> vision_mode_cache;
        // "Raw" night vision range - just stats+mutations+items
        float nv_range = 0;
        int sight_max;

        // turn the character expired, if calendar::before_time_starts it has not been set yet.
        // TODO: change into an optional<time_point>
        time_point time_died = calendar::before_time_starts;

        /**
         * Cache for pathfinding settings.
         * Most of it isn't changed too often, hence mutable.
         */
        mutable pimpl<pathfinding_settings> path_settings;

        // faction API versions
        // 2 - allies are in your_followers faction; NPCATT_FOLLOW is follower but not an ally
        // 0 - allies may be in your_followers faction; NPCATT_FOLLOW is an ally (legacy)
        // faction API versioning
        int faction_api_version = 2;
        // A temp variable used to inform the game which faction to link
        faction_id fac_id;
        faction *my_fac = nullptr;

        character_movemode move_mode;
        /** Current deficiency/excess quantity for each vitamin */
        std::map<vitamin_id, int> vitamin_levels;

        pimpl<player_morale> morale;

    private:
        /** suffer() subcalls */
        void suffer_water_damage( const mutation_branch &mdata );
        void suffer_mutation_power( const mutation_branch &mdata, Character::trait_data &tdata );
        void suffer_while_underwater();
        void suffer_from_addictions();
        void suffer_while_awake( int current_stim );
        void suffer_from_chemimbalance();
        void suffer_from_schizophrenia();
        void suffer_from_asthma( int current_stim );
        void suffer_from_pain();
        void suffer_in_sunlight();
        void suffer_from_albinism();
        void suffer_from_other_mutations();
        void suffer_from_radiation();
        void suffer_from_bad_bionics();
        void suffer_from_artifacts();
        void suffer_from_stimulants( int current_stim );
        void suffer_without_sleep( int sleep_deprivation );
        /**
         * Check whether the other creature is in range and can be seen by this creature.
         * @param critter Creature to check for visibility
         * @param range The maximal distance (@ref rl_dist), creatures at this distance or less
         * are included.
         */
        bool is_visible_in_range( const Creature &critter, int range ) const;

        // a cache of all active enchantment values.
        // is recalculated every turn in Character::recalculate_enchantment_cache
        enchantment enchantment_cache;
        player_activity destination_activity;
        // A unique ID number, assigned by the game class. Values should never be reused.
        character_id id;

        units::energy power_level;
        units::energy max_power_level;

        /** Needs (hunger, starvation, thirst, fatigue, etc.) */
        int stored_calories;

        int thirst;
        int stamina;

        int fatigue;
        int sleep_deprivation;
        bool check_encumbrance = true;

        int stim;
        int pkill;

        int radiation;

        std::vector<tripoint> auto_move_route;
        // Used to make sure auto move is canceled if we stumble off course
        cata::optional<tripoint> next_expected_position;
        scenttype_id type_of_scent;

        struct weighted_int_list<std::string> melee_miss_reasons;

        int cached_moves;
        tripoint cached_position;
        inventory cached_crafting_inventory;

    protected:
        /** Amount of time the player has spent in each overmap tile. */
        std::unordered_map<point, time_duration> overmap_time;

    public:
        // TODO: make these private
        std::array<int, num_bp> temp_cur, frostbite_timer, temp_conv;
        std::array<int, num_bp> body_wetness;
        std::array<int, num_bp> drench_capacity;

        time_point next_climate_control_check;
        bool last_climate_control_ret;
};

// Little size helper, exposed for use in deserialization code.
m_size calculate_size( const Character &c );

template<>
struct enum_traits<Character::stat> {
    static constexpr Character::stat last = Character::stat::DUMMY_STAT;
};
/**Get translated name of a stat*/
std::string get_stat_name( Character::stat Stat );
#endif // CATA_SRC_CHARACTER_H
