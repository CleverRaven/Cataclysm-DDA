#pragma once
#ifndef CATA_SRC_CHARACTER_H
#define CATA_SRC_CHARACTER_H

#include <functional>
#include <algorithm>
#include <bitset>
#include <climits>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <limits>
#include <list>
#include <map>
#include <new>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "activity_type.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character_id.h"
#include "coordinates.h"
#include "craft_command.h"
#include "creature.h"
#include "damage.h"
#include "enums.h"
#include "flat_set.h"
#include "game_constants.h"
#include "item.h"
#include "item_contents.h"
#include "item_location.h"
#include "item_pocket.h"
#include "magic_enchantment.h"
#include "memory_fast.h"
#include "optional.h"
#include "pimpl.h"
#include "player_activity.h"
#include "pldata.h"
#include "point.h"
#include "recipe.h"
#include "ret_val.h"
#include "stomach.h"
#include "string_formatter.h"
#include "type_id.h"
#include "units_fwd.h"
#include "visitable.h"
#include "weighted_list.h"

class Character;
class JsonIn;
class JsonObject;
class JsonOut;
class SkillLevel;
class SkillLevelMap;
class basecamp;
class bionic_collection;
class character_martial_arts;
class dispersion_sources;
class effect;
class effect_source;
class faction;
class inventory;
class known_magic;
class ma_technique;
class map;
class monster;
class nc_color;
class npc;
class player;
class player_morale;
class proficiency_set;
class recipe_subset;
class spell;
class vpart_reference;
struct bionic;
struct construction;
struct dealt_projectile_attack;
struct display_proficiency;
/// @brief Item slot used to apply modifications from food and meds
struct islot_comestible;
struct item_comp;
struct itype;
struct mutation_branch;
struct needs_rates;
struct pathfinding_settings;
struct points_left;
struct requirement_data;
struct tool_comp;
struct trap;
struct w_point;
template <typename E> struct enum_traits;

enum npc_attitude : int;
enum action_id : int;
enum class steed_type : int;

using drop_location = std::pair<item_location, int>;
using drop_locations = std::list<drop_location>;

constexpr int MAX_CLAIRVOYANCE = 40;

/// @brief type of conditions that effect vision
/// @note vision modes do not necessarily match json ids or flags
enum vision_modes {
    DEBUG_NIGHTVISION,
    NV_GOGGLES,
    NIGHTVISION_1,
    NIGHTVISION_2,
    NIGHTVISION_3,
    FULL_ELFA_VISION,
    ELFA_VISION,
    CEPH_VISION,
    /// mutate w/ id "FEL_NV" & name "Feline Vision" see pretty well at night
    FELINE_VISION,
    /// Bird mutation named "Avian Eyes": Perception +4
    BIRD_EYE,
    /// mutate w/ id "URSINE_EYE" & name "Ursine Vision" see better in dark, nearsight in light
    URSINE_VISION,
    BOOMERED,
    DARKNESS,
    IR_VISION,
    VISION_CLAIRVOYANCE,
    VISION_CLAIRVOYANCE_PLUS,
    VISION_CLAIRVOYANCE_SUPER,
    NUM_VISION_MODES
};

enum class fatigue_levels : int {
    TIRED = 191,
    DEAD_TIRED = 383,
    EXHAUSTED = 575,
    MASSIVE_FATIGUE = 1000
};
const std::unordered_map<std::string, fatigue_levels> fatigue_level_strs = { {
        { "TIRED", fatigue_levels::TIRED },
        { "DEAD_TIRED", fatigue_levels::DEAD_TIRED },
        { "EXHAUSTED", fatigue_levels::EXHAUSTED },
        { "MASSIVE_FATIGUE", fatigue_levels::MASSIVE_FATIGUE }
    }
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

/** @brief five levels of consequences for days without sleep
    @details Sleep deprivation, distinct from fatigue, is defined in minutes. Although most
    calculations scale linearly, malus is bestowed only upon reaching the tiers defined below.
    @note Sleep deprivation increases fatigue. Fatigue increase scales with the severity of sleep
    deprivation.
    @note Sleep deprivation kicks in if lack of sleep is avoided with stimulants or otherwise for
    long periods of time
    @see https://github.com/CleverRaven/Cataclysm-DDA/blob/master/src/character.cpp#L5566
*/
enum sleep_deprivation_levels {
    /// 2 days
    SLEEP_DEPRIVATION_HARMLESS = 2 * 24 * 60,
    /// 4 days
    SLEEP_DEPRIVATION_MINOR = 4 * 24 * 60,
    /// 7 days
    SLEEP_DEPRIVATION_SERIOUS = 7 * 24 * 60,
    /// 10 days
    SLEEP_DEPRIVATION_MAJOR = 10 * 24 * 60,
    /// 14 days
    SLEEP_DEPRIVATION_MASSIVE = 14 * 24 * 60
};

enum class blood_type {
    blood_O,
    blood_A,
    blood_B,
    blood_AB,
    num_bt
};

template<>
struct enum_traits<blood_type> {
    static constexpr blood_type last = blood_type::num_bt;
};

/// @brief how digestible or palatable an item is
/// @details This tries to represent both rating and character's decision to respect said rating
/// (ie "they can eat it, though they are disgusted by it")
enum edible_rating {
    /// Edible or we pretend it is
    EDIBLE,
    /// Not food at all
    INEDIBLE,
    /// Not food because character has mutated mouth/system
    INEDIBLE_MUTATION,
    /// You can eat it, but it will hurt morale because of negative trait such as Hates Fruit
    ALLERGY,
    /// Smaller allergy penalty
    ALLERGY_WEAK,
    /// Penalty for eating human flesh (unless psycho/cannibal)
    CANNIBALISM,
    /// Comestible has parasites
    PARASITES,
    /// Rotten (or, for saprophages, not rotten enough)
    ROTTEN,
    /// Can provoke vomiting if you already feel nauseous.
    NAUSEA,
    /// We can eat this, but we'll suffer from overeat
    TOO_FULL,
    /// Some weird stuff that requires a tool we don't have
    NO_TOOL
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

struct consumption_event {
    time_point time;
    itype_id type_id;
    uint64_t component_hash;

    consumption_event() = default;
    explicit consumption_event( const item &food ) : time( calendar::turn ) {
        type_id = food.typeId();
        component_hash = food.make_component_hash();
    }
    void serialize( JsonOut &json ) const;
    void deserialize( JsonIn &jsin );
};

struct weariness_tracker {
    int tracker = 0;
    int intake = 0;

    // Semi-consecutive 5 minute ticks of low activity (or 2.5 if we're sleeping)
    int low_activity_ticks = 0;
    // How many ticks since we've decreased intake
    int tick_counter = 0;

    void clear();
    void serialize( JsonOut &json ) const;
    void deserialize( JsonIn &jsin );
};

inline social_modifiers operator+( social_modifiers lhs, const social_modifiers &rhs )
{
    lhs += rhs;
    return lhs;
}

/// @brief Four ability stats available on character creation
/// @details  Default is 8. Minimum value is 4 and 14 is considered to be max human value.
enum class character_stat : char {
    STRENGTH,
    DEXTERITY,
    INTELLIGENCE,
    PERCEPTION,
    DUMMY_STAT
};

/**
 * Records a batch of unsealed containers and handles spilling at once. This
 * is preferred over handling containers right after unsealing because the latter
 * can cause items to be invalidated when later code still needs to access them.
 * See @ref `Character::handle_contents_changed` for more detail.
 */
class contents_change_handler
{
    public:
        contents_change_handler() = default;
        /**
         * Add an already unsealed container to the list. This item location
         * should remain valid when `handle_by` is called.
         */
        void add_unsealed( const item_location &loc );
        /**
         * Unseal the pocket containing `loc` and add `loc`'s parent to the list.
         * Does nothing if `loc` does not have a parent container. The parent of
         * `loc` should remain valid when `handle_by` is called, but `loc` only
         * needs to be valid here (for example, the item may be consumed afterwards).
         */
        void unseal_pocket_containing( const item_location &loc );
        /**
         * Let the guy handle any container that needs spilling. This may invalidate
         * items in and out of the list of containers. The list is cleared after handling.
         */
        void handle_by( Character &guy );
        /**
         * Serialization for activities
         */
        void serialize( JsonOut &jsout ) const;
        /**
         * Deserialization for activities
         */
        void deserialize( JsonIn &jsin );
    private:
        std::vector<item_location> unsealed;
};

enum class book_mastery {
    CANT_DETERMINE, // book not yet identified, so you don't know yet
    CANT_UNDERSTAND, // does not have enough skill to read
    LEARNING,
    MASTERED // can no longer increase skill by reading
};

class Character : public Creature, public visitable
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
        /// sets the ID, will *only* succeed when the current id is not valid
        /// allows forcing a -1 id which is required for templates to not throw errors
        void setID( character_id i, bool force = false );

        field_type_id bloodType() const override;
        field_type_id gibType() const override;
        bool is_warm() const override;
        bool in_species( const species_id &spec ) const override;
        /// Turned to false for simulating NPCs on distant missions so they don't drop all their gear in sight
        bool death_drops;
        /// Is currently in control of a vehicle
        bool controlling_vehicle = false;

        enum class comfort_level : int {
            impossible = -999,
            uncomfortable = -7,
            neutral = 0,
            slightly_comfortable = 3,
            comfortable = 5,
            very_comfortable = 10
        };

        /// @brief Character stats
        /// @todo Make those protected
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
        virtual int get_stored_kcal() const;
        virtual int get_healthy_kcal() const;
        virtual float get_kcal_percent() const;
        virtual int get_hunger() const;
        virtual int get_starvation() const;
        virtual int get_thirst() const;

        std::pair<std::string, nc_color> get_thirst_description() const;
        std::pair<std::string, nc_color> get_hunger_description() const;
        std::pair<std::string, nc_color> get_fatigue_description() const;
        std::pair<std::string, nc_color> get_weight_description() const;
        int get_fatigue() const;
        int get_sleep_deprivation() const;

        std::pair<std::string, nc_color> get_pain_description() const override;

        /** Modifiers for need values exclusive to characters */
        virtual void mod_stored_kcal( int nkcal, bool ignore_weariness = false );
        virtual void mod_stored_nutr( int nnutr );
        virtual void mod_hunger( int nhunger );
        virtual void mod_thirst( int nthirst );
        virtual void mod_fatigue( int nfatigue );
        virtual void mod_sleep_deprivation( int nsleep_deprivation );

        /** Setters for need values exclusive to characters */
        virtual void set_stored_kcal( int kcal );
        virtual void set_hunger( int nhunger );
        virtual void set_thirst( int nthirst );
        virtual void set_fatigue( int nfatigue );
        virtual void set_fatigue( fatigue_levels nfatigue );
        virtual void set_sleep_deprivation( int nsleep_deprivation );

    protected:

        // These accept values in calories, 1/1000s of kcals (or Calories)
        virtual void mod_stored_calories( int ncal, bool ignore_weariness = false );
        virtual void set_stored_calories( int cal );

    public:

        void mod_stat( const std::string &stat, float modifier ) override;

        int get_standard_stamina_cost( const item *thrown_item = nullptr ) const;

        /**Get bonus to max_hp from excess stored fat*/
        int get_fat_to_hp() const;

        /** Get size class of character **/
        creature_size get_size() const override;
        /** Recalculate size class of character **/
        void recalculate_size();

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

        // Has item with mission_id
        bool has_mission_item( int mission_id ) const;

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

        /* Gun stuff */
        /**
         * Check whether the player has a gun that uses the given type of ammo.
         */
        bool has_gun_for_ammo( const ammotype &at ) const;
        bool has_magazine_for_ammo( const ammotype &at ) const;

        /* Calculate aim improvement per move spent aiming at a given @ref recoil */
        double aim_per_move( const item &gun, double recoil ) const;

        /** Called after the player has successfully dodged an attack */
        void on_dodge( Creature *source, float difficulty ) override;

        /** Combat getters */
        float get_dodge_base() const override;
        /** Returns the player's dodge_roll to be compared against an aggressor's hit_roll() */
        float dodge_roll() const override;
        /** Returns Creature::get_dodge() modified by any Character effects */
        float get_dodge() const override;
        /** Handles the uncanny dodge bionic and effects, returns true if the player successfully dodges */
        bool uncanny_dodge() override;
        float get_hit_base() const override;

        const tripoint &pos() const override;
        /** Returns the player's sight range */
        int sight_range( int light_level ) const override;
        /** Returns the player maximum vision range factoring in mutations, diseases, and other effects */
        int  unimpaired_range() const;
        /** Returns true if overmap tile is within player line-of-sight */
        bool overmap_los( const tripoint_abs_omt &omt, int sight_points );
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
        /** Returns melee skill level, to be used to throttle dodge practice. **/
        float get_melee() const override;
        /**
         * @brief Adds a reason for why the player would miss a melee attack.
         *
         * @details To possibly be messaged to the player when he misses a melee attack.
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
        /// called once per 24 hours to enforce the minimum of 1 hp healed per day
        /// @todo Move to Character once heal() is moved
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
        /** Handles the chance to be infected by random diseases */
        void get_sick();
        /** Returns if the player has hibernation mutation and is asleep and well fed */
        bool is_hibernating() const;
        /** Maintains body temperature */
        void update_bodytemp();
        void update_frostbite( const bodypart_id &bp, int FBwindPower );
        /** Equalizes heat between body parts */
        void temp_equalizer( const bodypart_id &bp1, const bodypart_id &bp2 );

        struct comfort_response_t {
            comfort_level level = comfort_level::neutral;
            const item *aid = nullptr;
        };
        /** Rate point's ability to serve as a bed. Only takes certain mutations into account, and not fatigue nor stimulants. */
        comfort_response_t base_comfort_value( const tripoint &p ) const;

        /** Define blood loss (in percents) */
        int blood_loss( const bodypart_id &bp ) const;

        /** Resets the value of all bonus fields to 0. */
        void reset_bonuses() override;
        /** Resets stats, and applies effects in an idempotent manner */
        void reset_stats() override;
        /** Handles stat and bonus reset. */
        void reset() override;

        /** Returns ENC provided by armor, etc. */
        int encumb( const bodypart_id &bp ) const;

        /** Returns body weight plus weight of inventory and worn/wielded items */
        units::mass get_weight() const override;

        /** Returns true if the character is wearing power armor */
        bool is_wearing_power_armor( bool *hasHelmet = nullptr ) const;
        /** Returns true if the character is wearing active power */
        bool is_wearing_active_power_armor() const;
        /** Returns true if the player is wearing an active optical cloak */
        bool is_wearing_active_optcloak() const;

        /** Returns true if the player is in a climate controlled area or armor */
        bool in_climate_control();

        /** Returns wind resistance provided by armor, etc **/
        int get_wind_resistance( const bodypart_id &bp ) const;

        /** Returns true if the player isn't able to see */
        bool is_blind() const;

        bool is_invisible() const;
        /** Checks is_invisible() as well as other factors */
        int visibility( bool check_color = false, int stillness = 0 ) const;

        /** Returns character luminosity based on the brightest active item they are carrying */
        float active_light() const;

        bool sees_with_specials( const Creature &critter ) const;

        /** Bitset of all the body parts covered only with items with `flag` (or nothing) */
        body_part_set exclusive_flag_coverage( const flag_id &flag ) const;

        /** Processes effects which may prevent the Character from moving (bear traps, crushed, etc.).
         *  Returns false if movement is stopped. */
        bool move_effects( bool attacking ) override;

        void wait_effects( bool attacking = false );

        /** Series of checks to remove effects for waiting or moving */
        bool try_remove_grab();
        void try_remove_downed();
        void try_remove_bear_trap();
        void try_remove_lightsnare();
        void try_remove_heavysnare();
        void try_remove_crushed();
        void try_remove_webs();
        void try_remove_impeding_effect();

        /** Check against the character's current movement mode */
        bool movement_mode_is( const move_mode_id &mode ) const;
        move_mode_id current_movement_mode() const;

        bool is_running() const;
        bool is_walking() const;
        bool is_crouching() const;

        bool can_switch_to( const move_mode_id &mode ) const;
        steed_type get_steed_type() const;
        virtual void set_movement_mode( const move_mode_id &mode ) = 0;

        /**Determine if character is susceptible to dis_type and if so apply the symptoms*/
        void expose_to_disease( const diseasetype_id &dis_type );
        /**
         * Handles end-of-turn processing.
         */
        void process_turn() override;

        /** Recalculates HP after a change to max strength */
        void recalc_hp();
        int get_part_hp_max( const bodypart_id &id ) const;
        /** Modifies the player's sight values
         *  Must be called when any of the following change:
         *  This must be called when any of the following change:
         * - effects
         * - bionics
         * - traits
         * - underwater
         * - clothes
         */

        /** Maintains body wetness and handles the rate at which the player dries */
        void update_body_wetness( const w_point &weather );

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
        bool natural_attack_restricted_on( const bodypart_id &bp ) const;

        int blocks_left;
        int dodges_left;

        double recoil = MAX_RECOIL;

        std::string custom_profession;

        /** Returns true if the player is able to use a miss recovery technique */
        bool can_miss_recovery( const item &weap ) const;
        /** Returns true if the player has quiet melee attacks */
        bool is_quiet() const;

        // melee.cpp
        /** Checks for valid block abilities and reduces damage accordingly. Returns true if the player blocks */
        bool block_hit( Creature *source, bodypart_id &bp_hit, damage_instance &dam ) override;
        /** Returns the best item for blocking with */
        item &best_shield();
        /** Calculates melee weapon wear-and-tear through use, returns true if item is destroyed. */
        bool handle_melee_wear( item &shield, float wear_multiplier = 1.0f );
        /** Returns a random valid technique */
        matec_id pick_technique( Creature &t, const item &weap,
                                 bool crit, bool dodge_counter, bool block_counter );
        void perform_technique( const ma_technique &technique, Creature &t, damage_instance &di,
                                int &move_cost );

        // modifies the damage dealt based on the character's enchantments
        damage_instance modify_damage_dealt_with_enchantments( const damage_instance &dam ) const override;
        /**
         * Sets up a melee attack and handles melee attack function calls
         * @param t Creature to attack
         * @param allow_special whether non-forced martial art technique or mutation attack should be
         *   possible with this attack.
         * @param force_technique special technique to use in attack.
         * @param allow_unarmed always uses the wielded weapon regardless of martialarts style
         */
        bool melee_attack( Creature &t, bool allow_special, const matec_id &force_technique,
                           bool allow_unarmed = true );
        bool melee_attack_abstract( Creature &t, bool allow_special, const matec_id &force_technique,
                                    bool allow_unarmed = true );

        /** Handles reach melee attacks */
        void reach_attack( const tripoint &p );

        /**
         * Calls the to other melee_attack function with an empty technique id (meaning no specific
         * technique should be used).
         */
        bool melee_attack( Creature &t, bool allow_special );
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

        /** NPC-related item rating functions */
        double weapon_value( const item &weap, int ammo = 10 ) const; // Evaluates item as a weapon
        double gun_value( const item &weap, int ammo = 10 ) const; // Evaluates item as a gun
        double melee_value( const item &weap ) const; // As above, but only as melee
        double unarmed_value() const; // Evaluate yourself!
        /**
         * Returns a weapon's modified dispersion value.
         * @param obj Weapon to check dispersion on
         */
        dispersion_sources get_weapon_dispersion( const item &obj ) const;

        // If average == true, adds expected values of random rolls instead of rolling.
        /** Adds all 3 types of physical damage to instance */
        void roll_all_damage( bool crit, damage_instance &di, bool average, const item &weap ) const;
        /** Adds player's total bash damage to the damage instance */
        void roll_bash_damage( bool crit, damage_instance &di, bool average, const item &weap ) const;
        /** Adds player's total cut damage to the damage instance */
        void roll_cut_damage( bool crit, damage_instance &di, bool average, const item &weap ) const;
        /** Adds player's total stab damage to the damage instance */
        void roll_stab_damage( bool crit, damage_instance &di, bool average, const item &weap ) const;
        /** Adds player's total non-bash, non-cut, non-stab damage to the damage instance */
        void roll_other_damage( bool crit, damage_instance &di, bool average, const item &weap ) const;

        /** Returns true if the player should be dead */
        bool is_dead_state() const override;
        /** Returns true if the player has stealthy movement */
        bool is_stealthy() const;
        /** Returns true if the current martial art works with the player's current weapon */
        bool can_melee() const;
        /** Returns value of player's stable footing */
        float stability_roll() const override;
        /** Returns true if the player can learn the entered martial art */
        bool can_autolearn( const matype_id &ma_id ) const;
        /** Returns true if the player is able to use a grab breaking technique */
        bool can_grab_break( const item &weap ) const;
    private:
        /** Check if an area-of-effect technique has valid targets */
        bool valid_aoe_technique( Creature &t, const ma_technique &technique );
        bool valid_aoe_technique( Creature &t, const ma_technique &technique,
                                  std::vector<Creature *> &targets );
    public:

        /** This handles giving xp for a skill */
        void practice( const skill_id &id, int amount, int cap = 99, bool suppress_warning = false );
        /** This handles warning the player that there current activity will not give them xp */
        void handle_skill_warning( const skill_id &id, bool force_warning = false );

        /**
         * Check player capable of wielding an item.
         * @param it Thing to be wielded
         */
        ret_val<bool> can_wield( const item &it ) const;

        bool unwield();

        /** Get the formatted name of the currently wielded item (if any) with current gun mode (if gun) */
        std::string weapname() const;

        // any side effects that might happen when the Character is hit
        /** Handles special defenses from an attack that hit us (source can be null) */
        void on_hit( Creature *source, bodypart_id bp_hit,
                     float difficulty = INT_MIN, dealt_projectile_attack const *proj = nullptr ) override;
        // any side effects that might happen when the Character hits a Creature
        void did_hit( Creature &target );

        /** Actually hurt the player, hurts a body_part directly, no armor reduction */
        void apply_damage( Creature *source, bodypart_id hurt, int dam,
                           bool bypass_med = false ) override;
        /** Calls Creature::deal_damage and handles damaged effects (waking up, etc.) */
        dealt_damage_instance deal_damage( Creature *source, bodypart_id bp,
                                           const damage_instance &d ) override;
        /** Reduce healing effect intensity, return initial intensity of the effect */
        int reduce_healing_effect( const efftype_id &eff_id, int remove_med, const bodypart_id &hurt );

        void cough( bool harmful = false, int loudness = 4 );
        /**
         * Check for relevant passive, non-clothing that can absorb damage, and reduce by specified
         * damage unit.  Only flat bonuses are checked here.  Multiplicative ones are checked in
         * @ref player::absorb_hit.  The damage amount will never be reduced to less than 0.
         * This is called from @ref player::absorb_hit
         */
        void passive_absorb_hit( const bodypart_id &bp, damage_unit &du ) const;
        /** Runs through all bionics and armor on a part and reduces damage through their armor_absorb */
        void absorb_hit( const bodypart_id &bp, damage_instance &dam ) override;
        /**
         * Reduces and mutates du, prints messages about armor taking damage.
         * @return true if the armor was completely destroyed (and the item must be deleted).
         */
        bool armor_absorb( damage_unit &du, item &armor, const bodypart_id &bp );
        /**
         * Check for passive bionics that provide armor, and returns the armor bonus
         * This is called from player::passive_absorb_hit
         */
        float bionic_armor_bonus( const bodypart_id &bp, damage_type dt ) const;
        /** Returns the armor bonus against given type from martial arts buffs */
        int mabuff_armor_bonus( damage_type type ) const;
        /** Returns overall fire resistance for the body part */
        int get_armor_fire( const bodypart_id &bp ) const;
        // --------------- Mutation Stuff ---------------
        // In newcharacter.cpp
        /** Returns the id of a random starting trait that costs >= 0 points */
        trait_id random_good_trait();
        /** Returns the id of a random starting trait that costs < 0 points */
        trait_id random_bad_trait();
        /** Returns the id of a random trait matching the given predicate */
        trait_id get_random_trait( const std::function<bool( const mutation_branch & )> &func );
        void randomize_cosmetic_trait( std::string mutation_type );

        // In mutation.cpp
        /** Returns true if the player has a conflicting trait to the entered trait
         *  Uses has_opposite_trait(), has_lower_trait(), and has_higher_trait() to determine conflicts.
         */
        bool has_conflicting_trait( const trait_id &flag ) const;
        /** Returns true if the player has a trait which upgrades into the entered trait */
        bool has_lower_trait( const trait_id &flag ) const;
        /** Returns true if the player has a trait which is an upgrade of the entered trait */
        bool has_higher_trait( const trait_id &flag ) const;
        /** Returns true if the player has a trait that shares a type with the entered trait */
        bool has_same_type_trait( const trait_id &flag ) const;
        /** Returns true if the entered trait may be purified away
         *  Defaults to true
         */
        bool purifiable( const trait_id &flag ) const;
        /** Returns a dream's description selected randomly from the player's highest mutation category */
        std::string get_category_dream( const mutation_category_id &cat, int strength ) const;
        /** Returns true if the player has the entered trait */
        bool has_trait( const trait_id &b ) const override;
        /** Returns true if the player has the entered starting trait */
        bool has_base_trait( const trait_id &b ) const;
        /** Returns true if player has a trait with a flag */
        bool has_trait_flag( const json_character_flag &b ) const;
        /** Returns true if player has a bionic with a flag */
        bool has_bionic_with_flag( const json_character_flag &flag ) const;
        /** This is to prevent clang complaining about overloading a virtual function, the creature version uses monster flags so confusion is unlikely. */
        using Creature::has_flag;
        /** Returns true if player has a trait or bionic with a flag */
        bool has_flag( const json_character_flag &flag ) const;
        /** Returns the trait id with the given invlet, or an empty string if no trait has that invlet */
        trait_id trait_by_invlet( int ch ) const;

        /** Toggles a trait on the player and in their mutation list */
        void toggle_trait( const trait_id & );
        /** Add or removes a mutation on the player, but does not trigger mutation loss/gain effects. */
        void set_mutations( const std::vector<trait_id> &traits );
        void set_mutation( const trait_id & );
        void unset_mutation( const trait_id & );
        /**Unset switched mutation and set target mutation instead*/
        void switch_mutations( const trait_id &switched, const trait_id &target, bool start_powered );

        bool can_power_mutation( const trait_id &mut );
        /** Generates and handles the UI for player interaction with installed bionics */
        virtual void power_bionics() {}
        /**
        * Check whether player has a bionic power armor interface.
        * @return true if player has an active bionic capable of powering armor, false otherwise.
        */
        bool can_interface_armor() const;
        // TODO: Implement NPCs activating mutations
        virtual void power_mutations() {}

        /**Trigger reflex activation if the mutation has one*/
        void mutation_reflex_trigger( const trait_id &mut );

        // Trigger and disable mutations that can be so toggled.
        void activate_mutation( const trait_id &mutation );
        void deactivate_mutation( const trait_id &mut );

        bool can_mount( const monster &critter ) const;
        void mount_creature( monster &z );
        bool is_mounted() const;
        bool check_mount_will_move( const tripoint &dest_loc );
        bool check_mount_is_spooked();
        void dismount();
        void forced_dismount();

        bool is_deaf() const;
        bool is_mute() const;
        /** Returns true if the player has two functioning arms */
        bool has_two_arms() const;
        /** Returns the number of functioning arms */
        int get_working_arm_count() const;
        /** Returns the number of functioning legs */
        int get_working_leg_count() const;
        /** Returns true if the limb is disabled(12.5% or less hp)*/
        bool is_limb_disabled( const bodypart_id &limb ) const;
        /** Returns true if the limb is broken */
        bool is_limb_broken( const bodypart_id &limb ) const;
        /** source of truth of whether a Character can run */
        bool can_run() const;
        /** Hurts all body parts for dam, no armor reduction */
        void hurtall( int dam, Creature *source, bool disturb = true );
        /** Harms all body parts for dam, with armor reduction. If vary > 0 damage to parts are random within vary % (1-100) */
        int hitall( int dam, int vary, Creature *source );
        /** Handles effects that happen when the player is damaged and aware of the fact. */
        void on_hurt( Creature *source, bool disturb = true );
        /** Heals a body_part for dam */
        void heal_bp( bodypart_id bp, int dam ) override;
        /** Heals an part for dam */
        void heal( const bodypart_id &healed, int dam );
        /** Heals all body parts for dam */
        void healall( int dam );

        /** used for profession spawning and save migration for nested containers. remove after 0.F */
        void migrate_items_to_storage( bool disintegrate );

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
        bodypart_id body_window( const std::string &menu_header,
                                 bool show_all, bool precise,
                                 int normal_bonus, int head_bonus, int torso_bonus,
                                 int bleed, float bite, float infect, float bandage_power, float disinfectant_power ) const;

        // Returns color which this limb would have in healing menus
        nc_color limb_color( const bodypart_id &bp, bool bleed, bool bite, bool infect ) const;

        static const std::vector<material_id> fleshy;
        bool made_of( const material_id &m ) const override;
        bool made_of_any( const std::set<material_id> &ms ) const override;

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
            // In case we've moved out of range of lifting assist.
            invalidate_weight_carried_cache();
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
        tripoint_abs_omt global_omt_location() const;

    private:
        /** Retrieves a stat mod of a mutation. */
        int get_mod( const trait_id &mut, const std::string &arg ) const;
        /** Applies skill-based boosts to stats **/
        void apply_skill_boost();
        /**
          * What is the best pocket to put @it into?
          * the pockets in @avoid do not count
          */
        std::pair<item_location, item_pocket *> best_pocket( const item &it, const item *avoid );
    protected:

        void do_skill_rust();
        /** Applies stat mods to character. */
        void apply_mods( const trait_id &mut, bool add_remove );

        /** Applies encumbrance from mutations and bionics only */
        void mut_cbm_encumb( std::map<bodypart_id, encumbrance_data> &vals ) const;

        void apply_mut_encumbrance( std::map<bodypart_id, encumbrance_data> &vals ) const;

        /** Return the position in the worn list where new_item would be
         * put by default */
        std::list<item>::iterator position_to_wear_new_item( const item &new_item );

        /** Applies encumbrance from items only
         * If new_item is not null, then calculate under the asumption that it
         * is added to existing work items. */
        void item_encumb( std::map<bodypart_id, encumbrance_data> &vals, const item &new_item ) const;

    public:
        /** Recalculate encumbrance for all body parts. */
        void calc_encumbrance();
        /** Recalculate encumbrance for all body parts as if `new_item` was also worn. */
        void calc_encumbrance( const item &new_item );

        // recalculates enchantment cache by iterating through all held, worn, and wielded items
        void recalculate_enchantment_cache();
        // gets add and mult value from enchantment cache
        double calculate_by_enchantment( double modify, enchant_vals::mod value,
                                         bool round_output = false ) const;

        /** Returns true if the player has any martial arts buffs attached */
        bool has_mabuff( const mabuff_id &buff_id ) const;
        /** Returns true if the player has a grab breaking technique available */
        bool has_grab_break_tec() const override;

        /** Returns the to hit bonus from martial arts buffs */
        float mabuff_tohit_bonus() const;
        /** Returns the critical hit chance bonus from martial arts buffs */
        float mabuff_critical_hit_chance_bonus() const;
        /** Returns the dodge bonus from martial arts buffs */
        float mabuff_dodge_bonus() const;
        /** Returns the blocking effectiveness bonus from martial arts buffs */
        int mabuff_block_effectiveness_bonus() const;
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
        void mutation_effect( const trait_id &mut, bool worn_destroyed_override );
        /** Handles what happens when you lose a mutation. */
        void mutation_loss_effect( const trait_id &mut );

        bool has_active_mutation( const trait_id &b ) const;

        int get_cost_timer( const trait_id &mut_id ) const;
        void set_cost_timer( const trait_id &mut, int set );
        void mod_cost_timer( const trait_id &mut, int mod );

        /** Picks a random valid mutation and gives it to the Character, possibly removing/changing others along the way */
        void mutate();
        /** Returns true if the player doesn't have the mutation or a conflicting one and it complies with the force typing */
        bool mutation_ok( const trait_id &mutation, bool force_good, bool force_bad ) const;
        /** Picks a random valid mutation in a category and mutate_towards() it */
        void mutate_category( const mutation_category_id &mut_cat );
        /** Mutates toward one of the given mutations, upgrading or removing conflicts if necessary */
        bool mutate_towards( std::vector<trait_id> muts, int num_tries = INT_MAX );
        /** Mutates toward the entered mutation, upgrading or removing conflicts if necessary */
        bool mutate_towards( const trait_id &mut );
        /** Removes a mutation, downgrading to the previous level if possible */
        void remove_mutation( const trait_id &mut, bool silent = false );
        /** Returns true if the player has the entered mutation child flag */
        bool has_child_flag( const trait_id &flag ) const;
        /** Removes the mutation's child flag from the player's list */
        void remove_child_flag( const trait_id &flag );
        /** Recalculates mutation_category_level[] values for the player */
        void set_highest_cat_level();
        /** Returns the highest mutation category */
        mutation_category_id get_highest_category() const;
        /** Recalculates mutation drench protection for all bodyparts (ignored/good/neutral stats) */
        void drench_mut_calc();
        /** Recursively traverses the mutation's prerequisites and replacements, building up a map */
        void build_mut_dependency_map( const trait_id &mut,
                                       std::unordered_map<trait_id, int> &dependency_map, int distance );

        /**
        * Returns true if this category of mutation is allowed.
        */
        bool is_category_allowed( const std::vector<mutation_category_id> &category ) const;
        bool is_category_allowed( const mutation_category_id &category ) const;

        bool is_weak_to_water() const;

        /**Check for mutation disallowing the use of an healing item*/
        bool can_use_heal_item( const item &med ) const;

        bool can_install_cbm_on_bp( const std::vector<bodypart_id> &bps ) const;

        /// @brief Returns resistances on a body part provided by mutations
        /// @todo Cache this, it's kinda expensive to compute
        resistances mutation_armor( bodypart_id bp ) const;
        float mutation_armor( bodypart_id bp, damage_type dt ) const;
        float mutation_armor( bodypart_id bp, const damage_unit &du ) const;

        // --------------- Bionic Stuff ---------------
        /** Handles bionic activation effects of the entered bionic, returns if anything activated */
        bool activate_bionic( int b, bool eff_only = false, bool *close_bionics_ui = nullptr );
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
        std::vector<bionic_id> get_bionic_fueled_with( const material_id &mat ) const;
        /**Return bionic_id of fueled bionics*/
        std::vector<bionic_id> get_fueled_bionics() const;
        /**Returns bionic_id of first remote fueled bionic found*/
        bionic_id get_remote_fueled_bionic() const;
        /**Return bionic_id of bionic of most fuel efficient bionic*/
        bionic_id get_most_efficient_bionic( const std::vector<bionic_id> &bids ) const;
        /**Return list of available fuel for this bionic*/
        std::vector<material_id> get_fuel_available( const bionic_id &bio ) const;
        /**Return available space to store specified fuel*/
        int get_fuel_capacity( const material_id &fuel ) const;
        /**Return total space to store specified fuel*/
        int get_total_fuel_capacity( const material_id &fuel ) const;
        /**Updates which bionic contain fuel and which is empty*/
        void update_fuel_storage( const material_id &fuel );
        /**Get stat bonus from bionic*/
        int get_mod_stat_from_bionic( const character_stat &Stat ) const;
        // route for overmap-scale traveling
        std::vector<tripoint_abs_omt> omt_path;

        /** Handles bionic effects over time of the entered bionic */
        void process_bionic( int b );
        /** finds the index of the bionic that corresponds to the currently wielded fake item
         *  i.e. bionic is `BIONIC_WEAPON` and weapon.typeId() == bio.info().fake_item */
        cata::optional<int> active_bionic_weapon_index() const;
        /** Checks if bionic can be deactivated (e.g. it's not incapacitated and power level is sufficient)
         *  returns either success or failure with log message */
        ret_val<bool> can_deactivate_bionic( int b, bool eff_only = false ) const;
        /** Handles bionic deactivation effects of the entered bionic, returns if anything
         *  deactivated */
        bool deactivate_bionic( int b, bool eff_only = false );
        /** Returns the size of my_bionics[] */
        int num_bionics() const;
        /** Returns the bionic at a given index in my_bionics[] */
        bionic &bionic_at_index( int i );
        /** Remove all bionics */
        void clear_bionics();
        int get_used_bionics_slots( const bodypart_id &bp ) const;
        int get_total_bionics_slots( const bodypart_id &bp ) const;
        int get_free_bionics_slots( const bodypart_id &bp ) const;

        /**Has enough anesthetic for surgery*/
        bool has_enough_anesth( const itype &cbm, player &patient );
        bool has_enough_anesth( const itype &cbm );
        void consume_anesth_requirement( const itype &cbm, player &patient );
        /**Has the required equipment for manual installation*/
        bool has_installation_requirement( const bionic_id &bid );
        void consume_installation_requirement( const bionic_id &bid );
        /** Handles process of introducing patient into anesthesia during Autodoc operations. Requires anesthesia kits or NOPAIN mutation */
        void introduce_into_anesthesia( const time_duration &duration, player &installer,
                                        bool needs_anesthesia );
        /** Removes a bionic from my_bionics[] */
        void remove_bionic( const bionic_id &b );
        /** Adds a bionic to my_bionics[] */
        void add_bionic( const bionic_id &b );
        /**Calculate skill bonus from tiles in radius*/
        float env_surgery_bonus( int radius ) const;
        /** Calculate skill for (un)installing bionics */
        float bionics_adjusted_skill( bool autodoc, int skill_level = -1 ) const;
        /** Calculate non adjusted skill for (un)installing bionics */
        int bionics_pl_skill( bool autodoc, int skill_level = -1 ) const;
        /**Is the installation possible*/
        bool can_install_bionics( const itype &type, Character &installer, bool autodoc = false,
                                  int skill_level = -1 );
        std::map<bodypart_id, int> bionic_installation_issues( const bionic_id &bioid );
        /** Initialize all the values needed to start the operation player_activity */
        bool install_bionics( const itype &type, player &installer, bool autodoc = false,
                              int skill_level = -1 );
        /**Success or failure of installation happens here*/
        void perform_install( const bionic_id &bid, const bionic_id &upbid, int difficulty, int success,
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
        /**Success or failure of removal happens here*/
        void perform_uninstall( const bionic_id &bid, int difficulty, int success,
                                const units::energy &power_lvl, int pl_skill );
        /**When a player fails the surgery*/
        void bionics_uninstall_failure( int difficulty, int success, float adjusted_skill );

        /**When a critical failure occurs*/
        void roll_critical_bionics_failure( const bodypart_id &bp );

        /**Used by monster to perform surgery*/
        bool uninstall_bionic( const bionic &target_cbm, monster &installer, player &patient,
                               float adjusted_skill );
        /**When a monster fails the surgery*/
        void bionics_uninstall_failure( monster &installer, player &patient, int difficulty, int success,
                                        float adjusted_skill );

        /**Passively produce power from PERPETUAL fuel*/
        void passive_power_gen( int b );
        /**Find fuel used by remote powered bionic*/
        material_id find_remote_fuel( bool look_only = false );
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

        void conduct_blood_analysis();
        // --------------- Generic Item Stuff ---------------

        struct has_mission_item_filter {
            int mission_id;
            bool operator()( const item &it ) {
                return it.mission_id == mission_id || it.contents.has_any_with( [&]( const item & it ) {
                    return it.mission_id == mission_id;
                }, item_pocket::pocket_type::SOFTWARE );
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
        virtual bool invoke_item( item *, const tripoint &pt, int pre_obtain_moves = -1 );
        /** As above, but with a pre-selected method. Debugmsg if this item doesn't have this method. */
        virtual bool invoke_item( item *, const std::string &, const tripoint &pt,
                                  int pre_obtain_moves = -1 );
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
         * Remove charges from a specific item.
         * The item must exist and it must be counted by charges.
         * @param it A pointer to the item, it *must* exist.
         * @param quantity The number of charges to remove, must not be larger than
         * the current charges of the item.
         * @return An item that contains the removed charges, it's effectively a
         * copy of the item with the proper charges.
         */
        item reduce_charges( item *it, int quantity );

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

        /**
         * Calculate (but do not deduct) the number of moves required when drawing a weapon from an holster or sheathe
         * @param it Item to calculate retrieve cost for
         * @param container Container where the item is
         * @param penalties Whether item volume and temporary effects (e.g. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         * @return cost in moves ranging from 0 to MAX_HANDLING_COST
         */
        int item_retrieve_cost( const item &it, const item &container, bool penalties = true,
                                int base_cost = INVENTORY_HANDLING_PENALTY ) const;

        /** Calculate (but do not deduct) the number of moves required to wear an item */
        int item_wear_cost( const item &it ) const;

        /** Wear item; returns nullopt on fail, or pointer to newly worn item on success.
         * If interactive is false, don't alert the player or drain moves on completion.
         * If do_calc_encumbrance is false, don't recalculate encumbrance, caller must call it eventually.
         */
        cata::optional<std::list<item>::iterator>
        wear_item( const item &to_wear, bool interactive = true, bool do_calc_encumbrance = true );

        /** Returns the amount of item `type' that is currently worn */
        int  amount_worn( const itype_id &id ) const;

        /** Returns the amount of software `type' that are in the inventory */
        int count_softwares( const itype_id &id );

        /** Returns nearby items which match the provided predicate */
        std::vector<item_location> nearby( const std::function<bool( const item *, const item * )> &func,
                                           int radius = 1 ) const;

        /**
         * Similar to @ref remove_items_with, but considers only worn items and not their
         * content (@ref item::contents is not checked).
         * If the filter function returns true, the item is removed.
         */
        std::list<item> remove_worn_items_with( const std::function<bool( item & )> &filter );

        // returns a list of all item_location the character has, including items contained in other items.
        // only for CONTAINER pocket type; does not look for magazines
        std::vector<item_location> all_items_loc();
        // Returns list of all the top level item_lodation the character has. Includes worn items but excludes items held on hand.
        std::vector<item_location> top_items_loc();
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
         * Adds the item to the character's worn items or wields it, or prompts if the Character cannot pick it up.
         * @avoid is the item to not put @it into
         */
        item &i_add( item it, bool should_stack = true, const item *avoid = nullptr, bool allow_drop = true,
                     bool allow_wield = true );
        /** tries to add to the character's inventory without a popup. returns nullptr if it fails. */
        item *try_add( item it, const item *avoid = nullptr, bool allow_wield = true );

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
        bool pour_into( const vpart_reference &vp, item &liquid ) const;
        /**@}*/

        /**
         * Remove a specific item from player possession. The item is compared
         * by pointer. Contents of the item are removed as well.
         * @param it A pointer to the item to be removed. The item *must* exists
         * in the players possession (one can use @ref has_item to check for this).
         * @return A copy of the removed item.
         */
        item i_rem( const item *it );
        void i_rem_keep_contents( const item *it );
        /** Sets invlet and adds to inventory if possible, drops otherwise, returns true if either succeeded.
         *  An optional qty can be provided (and will perform better than separate calls). */
        bool i_add_or_drop( item &it, int qty = 1, const item *avoid = nullptr );

        /**
         * Check any already unsealed pockets in items pointed to by `containers`
         * and propagate the unsealed status through the container tree. In the
         * process the player may be asked to handle containers or spill contents,
         * so make sure all unsealed containers are passed to this fucntion in a
         * single batch; items (not limited to the ones listed in `containers` and
         * their contents) may be invalidated or moved after a call to this function.
         *
         * @param containers Item locations pointing to containers. Item locations
         *        in this vector can contain each other, but should always be valid
         *        (i.e. if A contains B and B contains C, A and C can be in the vector
         *        at the same time and should be valid, but B shouldn't be invalidated
         *        either, otherwise C is invalidated). Item location in this vector
         *        should be unique.
         */
        void handle_contents_changed( const std::vector<item_location> &containers );

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

        /**
         * Whether a tool or gun is potentially reloadable (optionally considering a specific ammo)
         * @param it Thing to be reloaded
         * @param ammo if set also check item currently compatible with this specific ammo or magazine
         * @note items currently loaded with a detachable magazine are considered reloadable
         * @note items with integral magazines are reloadable if free capacity permits (+/- ammo matches)
         */
        bool can_reload( const item &it, const itype_id &ammo = itype_id() ) const;

        /** Same as `Character::can_reload`, but checks for attached gunmods as well. */
        hint_rating rate_action_reload( const item &it ) const;
        /** Whether a tool or a gun can be unloaded. */
        hint_rating rate_action_unload( const item &it ) const;
        /**
          * So far only called by unload() from game.cpp
          * @avoid - do not put @it into @avoid
          */
        bool add_or_drop_with_msg( item &it, bool unloading = false, const item *avoid = nullptr );
        /**
         * Unload item.
         * @param bypass_activity If item requires an activity for its unloading, unload item immediately instead.
         */
        bool unload( item_location &loc, bool bypass_activity = false );
        /**
         * Calculate (but do not deduct) the number of moves required to reload an item with specified quantity of ammo
         * @param it Item to calculate reload cost for
         * @param ammo either ammo or magazine to use when reloading the item
         * @param qty maximum units of ammo to reload. Capped by remaining capacity and ignored if reloading using a magazine.
         */
        int item_reload_cost( const item &it, const item &ammo, int qty ) const;

        projectile thrown_item_projectile( const item &thrown ) const;
        int thrown_item_adjusted_damage( const item &thrown ) const;
        // calculates the total damage possible from a thrown item, without resistances and such.
        int thrown_item_total_damage_raw( const item &thrown ) const;
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
        /// returning the largest weight liftable by an item in range.
        units::mass best_nearby_lifting_assist() const;

        /// Alternate version if you need to specify a different origin point for nearby vehicle sources of lifting
        /// used for operations on distant objects (e.g. vehicle installation/uninstallation)
        units::mass best_nearby_lifting_assist( const tripoint &world_pos ) const;

        // Inventory + weapon + worn (for death, etc)
        std::vector<item *> inv_dump();

        units::mass weight_carried() const;
        units::volume volume_carried() const;

        units::length max_single_item_length() const;
        units::volume max_single_item_volume() const;

        /// Sometimes we need to calculate hypothetical volume or weight.  This
        /// struct offers two possible tweaks: a collection of items and
        /// counts to remove, or an entire replacement inventory.
        struct item_tweaks {
            item_tweaks() = default;
            explicit item_tweaks( const std::map<const item *, int> &w ) :
                without_items( std::cref( w ) )
            {}
            explicit item_tweaks( const inventory &r ) :
                replace_inv( std::cref( r ) )
            {}
            const cata::optional<std::reference_wrapper<const std::map<const item *, int>>> without_items;
            const cata::optional<std::reference_wrapper<const inventory>> replace_inv;
        };

        units::mass weight_carried_with_tweaks( const item_tweaks &tweaks ) const;
        units::mass weight_carried_with_tweaks( const std::vector<std::pair<item_location, int>>
                                                &locations ) const;
        units::mass get_selected_stack_weight( const item *i,
                                               const std::map<const item *, int> &without ) const;
        units::volume volume_carried_with_tweaks( const item_tweaks &tweaks ) const;
        units::volume volume_carried_with_tweaks( const std::vector<std::pair<item_location, int>>
                &locations ) const;
        units::mass weight_capacity() const override;
        units::volume volume_capacity() const;
        units::volume volume_capacity_with_tweaks( const item_tweaks &tweaks ) const;
        units::volume volume_capacity_with_tweaks( const std::vector<std::pair<item_location, int>>
                &locations ) const;
        units::volume free_space() const;

        /** Note that we've read a book at least once. **/
        virtual bool has_identified( const itype_id &item_id ) const = 0;
        book_mastery get_book_mastery( const item &book ) const;
        virtual void identify( const item &item ) = 0;
        /** Calculates the total fun bonus relative to this character's traits and chapter progress */
        bool fun_to_read( const item &book ) const;
        int book_fun_for( const item &book, const Character &p ) const;

        bool can_pickVolume( const item &it, bool safe = false ) const;
        bool can_pickWeight( const item &it, bool safe = true ) const;
        bool can_pickWeight_partial( const item &it, bool safe = true ) const;
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
        * Returns true if the character is wielding something and it can't be combined with the item
        * passed as a parameter
        */
        bool has_wield_conflicts( const item &it ) const;

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
        /**
         * Check player capable of dropping an item.
         * @param it Thing to be unwielded
         */
        ret_val<bool> can_drop( const item &it ) const;

        void drop_invalid_inventory();
        // this cache is for checking if items in the character's inventory can't actually fit into other items they are inside of
        void invalidate_inventory_validity_cache();

        void invalidate_weight_carried_cache();
        /** Returns all items that must be taken off before taking off this item */
        std::list<item *> get_dependent_worn_items( const item &it );
        /** Drops an item to the specified location */
        void drop( item_location loc, const tripoint &where );
        virtual void drop( const drop_locations &what, const tripoint &target, bool stash = false );

        bool is_wielding( const item &target ) const;

        bool covered_with_flag( const flag_id &flag, const body_part_set &parts ) const;
        bool is_waterproof( const body_part_set &parts ) const;
        // Carried items may leak radiation or chemicals
        int leak_level( const flag_id &flag ) const;

        // --------------- Clothing Stuff ---------------
        /** Returns true if the player is wearing the item. */
        bool is_wearing( const itype_id &it ) const;
        /** Returns true if the player is wearing the item on the given body part. */
        bool is_wearing_on_bp( const itype_id &it, const bodypart_id &bp ) const;
        /** Returns true if the player is wearing an item with the given flag. */
        bool worn_with_flag( const flag_id &flag, const bodypart_id &bp ) const;
        bool worn_with_flag( const flag_id &flag ) const;
        /** Returns the first worn item with a given flag. */
        item item_worn_with_flag( const flag_id &flag, const bodypart_id &bp ) const;
        item item_worn_with_flag( const flag_id &flag ) const;

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
        /** Returns a value used when attempting to convince NPC's of something */
        int talk_skill() const;
        /** Returns a value used when attempting to intimidate NPC's */
        int intimidation() const;

        // --------------- Proficiency Stuff ----------------
        bool has_proficiency( const proficiency_id &prof ) const;
        bool has_prof_prereqs( const proficiency_id &prof ) const;
        void add_proficiency( const proficiency_id &prof, bool ignore_requirements = false );
        void lose_proficiency( const proficiency_id &prof, bool ignore_requirements = false );
        void practice_proficiency( const proficiency_id &prof, const time_duration &amount,
                                   const cata::optional<time_duration> &max = cata::nullopt );
        time_duration proficiency_training_needed( const proficiency_id &prof ) const;
        std::vector<display_proficiency> display_proficiencies() const;
        std::vector<proficiency_id> known_proficiencies() const;
        std::vector<proficiency_id> learning_proficiencies() const;

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
        pimpl<known_magic> magic;

        // gets all the spells known by this character that have this spell class
        // spells returned are a copy, do not try to edit them from here, instead use known_magic::get_spell
        std::vector<spell> spells_known_of_class( const trait_id &spell_class ) const;
        bool cast_spell( spell &sp, bool fake_spell, cata::optional<tripoint> target );

        void make_bleed( const effect_source &source, const bodypart_id &bp, time_duration duration,
                         int intensity = 1, bool permanent = false, bool force = false, bool defferred = false );

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
        float healing_rate_medicine( float at_rest_quality, const bodypart_id &bp ) const;

        /**
         * Goes over all mutations, gets min and max of a value with given name
         * @return min( 0, lowest ) + max( 0, highest )
         */
        float mutation_value( const std::string &val ) const;

        /**
         * Goes over all mutations/bionics, returning the sum of the social modifiers
         */
        social_modifiers get_mutation_bionic_social_mods() const;

        // Display
        nc_color symbol_color() const override;
        const std::string &symbol() const override;

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
        bool male = false;

        std::list<item> worn;
        bool nv_cached = false;
        // Means player sit inside vehicle on the tile he is now
        bool in_vehicle = false;
        bool hauling = false;

        player_activity stashed_outbounds_activity;
        player_activity stashed_outbounds_backlog;
        player_activity activity;
        std::list<player_activity> backlog;
        cata::optional<tripoint> destination_point;
        pimpl<inventory> inv;
        itype_id last_item;
        item weapon;

        int scent = 0;
        pimpl<bionic_collection> my_bionics;
        pimpl<character_martial_arts> martial_arts_data;

        stomach_contents stomach;
        stomach_contents guts;
        std::list<consumption_event> consumption_history;

        int oxygen = 0;
        int slow_rad = 0;
        blood_type my_blood_type;
        bool blood_rh_factor = false;
        // Randomizes characters' blood type and Rh
        void randomize_blood();

        int get_focus() const {
            return std::max( 1, focus_pool / 1000 );
        }
        void mod_focus( int amount ) {
            focus_pool += amount * 1000;
            focus_pool = std::max( focus_pool, 0 );
        }
        // Set the focus pool directly, only use for debugging.
        void set_focus( int amount ) {
            focus_pool = amount * 1000;
        }
    protected:
        int focus_pool = 0;
    public:
        int cash = 0;
        std::set<character_id> follower_ids;
        weak_ptr_fast<Creature> last_target;
        cata::optional<tripoint> last_target_pos;
        // Save favorite ammo location
        item_location ammo_location;
        std::set<tripoint_abs_omt> camps;
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
        int addiction_level( add_type type ) const;

        shared_ptr_fast<monster> mounted_creature;
        // for loading NPC mounts
        int mounted_creature_id = 0;
        // for vehicle work
        int activity_vehicle_part_index = -1;

        // Hauling items on the ground
        void start_hauling();
        void stop_hauling();
        bool is_hauling() const;

        // Has a weapon, inventory item or worn item with flag
        bool has_item_with_flag( const flag_id &flag, bool need_charges = false ) const;
        /**
         * All items that have the given flag (@ref item::has_flag).
         */
        std::vector<const item *> all_items_with_flag( const flag_id &flag ) const;

        bool has_charges( const itype_id &it, int quantity,
                          const std::function<bool( const item & )> &filter = return_true<item> ) const override;

        // has_amount works ONLY for quantity.
        // has_charges works ONLY for charges.
        std::list<item> use_amount( const itype_id &it, int quantity,
                                    const std::function<bool( const item & )> &filter = return_true<item> );
        // Uses up charges
        bool use_charges_if_avail( const itype_id &it, int quantity );

        /**
        * Use charges in character inventory.
        * @param what itype_id of item using charges
        * @param qty Number of charges
        * @param filter Filter
        * @return List of items used
        */
        std::list<item> use_charges( const itype_id &what, int qty,
                                     const std::function<bool( const item & )> &filter = return_true<item> );
        /**
        * Use charges within a radius. Includes character inventory.
        * @param what itype_id of item using charges
        * @param qty Number of charges
        * @param radius Radius from the character. Use -1 to use from character inventory only.
        * @param filter Filter
        * @return List of items used
        */
        std::list<item> use_charges( const itype_id &what, int qty, int radius,
                                     const std::function<bool( const item & )> &filter = return_true<item> );

        const item *find_firestarter_with_charges( int quantity ) const;
        bool has_fire( int quantity ) const;
        void use_fire( int quantity );
        void assign_stashed_activity();
        bool check_outbounds_activity( const player_activity &act, bool check_only = false );
        /// @warning Legacy activity assignment, does not work for any activities using
        /// the new activity_actor class and may cause issues with resuming.
        /// @todo delete this once migration of activities to the activity_actor system is complete
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
        bool can_stash( const item &it );
        bool can_stash_partial( const item &it );
        void initialize_stomach_contents();

        /** Stable base metabolic rate due to traits */
        float metabolic_rate_base() const;
        /** Current metabolic rate due to traits, hunger, speed, etc. */
        float metabolic_rate() const;
        // gets the max value healthy you can be, related to your weight
        int get_max_healthy() const;
        // gets the string that describes your weight
        std::string get_weight_string() const;
        // gets the description, printed in player_display, related to your current bmi
        std::string get_weight_long_description() const;
        // calculates the BMI
        float get_bmi() const;
        // returns amount of calories burned in a day given various metabolic factors
        int get_bmr() const;
        // add spent calories to calorie diary (if avatar)
        virtual void add_spent_calories( int /* cal */ ) {}
        // add gained calories to calorie diary (if avatar)
        virtual void add_gained_calories( int /* gained */ ) {}
        // log the activity level in the calorie diary (if avatar)
        virtual void log_activity_level( float /*level*/ ) {}
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
        // increases the activity level to the next level
        // does not decrease activity level
        void increase_activity_level( float new_level );
        // decreases the activity level to the previous level
        // does not increase activity level
        void decrease_activity_level( float new_level );
        // sets activity level to NO_EXERCISE
        void reset_activity_level();
        // outputs player activity level to a printable string
        std::string activity_level_str() const;
        // NOT SUITABLE FOR USE OTHER THAN DISPLAY
        // The activity level this turn
        float instantaneous_activity_level() const;
        // Basically, advance this display one turn
        void reset_activity_cursor();
        // When we log an activity for metabolic purposes
        // log it in our cursor too
        void log_instant_activity( float );

        /** Returns overall bashing resistance for the body_part */
        int get_armor_bash( bodypart_id bp ) const override;
        /** Returns overall cutting resistance for the body_part */
        int get_armor_cut( bodypart_id bp ) const override;
        /** Returns overall bullet resistance for the body_part */
        int get_armor_bullet( bodypart_id bp ) const override;
        /** Returns bashing resistance from the creature and armor only */
        int get_armor_bash_base( bodypart_id bp ) const override;
        /** Returns cutting resistance from the creature and armor only */
        int get_armor_cut_base( bodypart_id bp ) const override;
        /** Returns cutting resistance from the creature and armor only */
        int get_armor_bullet_base( bodypart_id bp ) const override;
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
        void on_damage_of_type( int adjusted_damage, damage_type type, const bodypart_id &bp ) override;
    public:
        /** Called when an item is worn */
        void on_item_wear( const item &it );
        /** Called when an item is taken off */
        void on_item_takeoff( const item &it );
        /** Called when an item is washed */
        void on_worn_item_washed( const item &it );
        /** Called when effect intensity has been changed */
        void on_effect_int_change( const efftype_id &eid, int intensity,
                                   const bodypart_id &bp = bodypart_id( "bp_null" ) ) override;
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

        std::map<mutation_category_id, int> mutation_category_level;

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
        void place_corpse( const tripoint_abs_omt &om_target );

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
        /**
         * As above, but includes all creatures the player can detect well enough to target
         * with ranged weapons, e.g. with infrared vision.
         */
        std::vector<Creature *> get_targetable_creatures( int range, bool melee ) const;
        /** Returns an enumeration of visible mutations with colors */
        std::string visible_mutations( int visibility_cap ) const;
        player_activity get_destination_activity() const;
        void set_destination_activity( const player_activity &new_destination_activity );
        void clear_destination_activity();
        /** Returns warmth provided by armor, etc. */
        int warmth( const bodypart_id &bp ) const;
        /** Returns warmth provided by an armor's bonus, like hoods, pockets, etc. */
        int bonus_item_warmth( const bodypart_id &bp ) const;
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

        bool in_sleep_state() const override;
        /** Set vitamin deficiency/excess disease states dependent upon current vitamin levels */
        void update_vitamins( const vitamin_id &vit );
        /**
         * @brief Check current level of a vitamin
         *
         * @details Accesses level of a given vitamin.  If the vitamin_id specified does not
         * exist then this function simply returns 0.
         *
         * @param vit ID of vitamin to check level for (ie "vitA", "vitB").
         * @returns character's current level for specified vitamin
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
        /** Can the food be [theoretically] eaten no matter the consequences? */
        ret_val<edible_rating> can_eat( const item &food ) const;
        /**
         * Same as @ref can_eat, but takes consequences into account.
         * Asks about them if @param interactive is true, refuses otherwise.
         */
        ret_val<edible_rating> will_eat( const item &food, bool interactive = false ) const;
        /** Determine character's capability of recharging their CBMs. */
        int get_acquirable_energy( const item &it ) const;

        /**
        * Recharge CBMs whenever possible.
        * @return true when recharging was successful.
        */
        bool fuel_bionic_with( item &it );
        /** Used to apply stimulation modifications from food and medication **/
        void modify_stimulation( const islot_comestible &comest );
        /** Used to apply fatigue modifications from food and medication **/
        /** Used to apply radiation from food and medication **/
        void modify_fatigue( const islot_comestible &comest );
        void modify_radiation( const islot_comestible &comest );
        /** Used to apply addiction modifications from food and medication **/
        void modify_addiction( const islot_comestible &comest );
        /** Used to apply health modifications from food and medication **/
        void modify_health( const islot_comestible &comest );
        /** Used to compute how filling a food is.*/
        double compute_effective_food_volume_ratio( const item &food ) const;
        /** Used to calculate dry volume of a chewed food **/
        units::volume masticated_volume( const item &food ) const;
        /** Used to to display how filling a food is. */
        int compute_calories_per_effective_volume( const item &food,
                const nutrients *nutrient = nullptr ) const;
        /** Handles the effects of consuming an item */
        bool consume_effects( item &food );
        /** Check whether the character can consume this very item */
        bool can_consume_as_is( const item &it ) const;
        /** Check whether the character can consume this item or any of its contents */
        bool can_consume( const item &it ) const;
        /** True if the character has enough skill (in cooking or survival) to estimate time to rot */
        bool can_estimate_rot() const;
        /**
         * Returns a reference to the item itself (if it's consumable),
         * the first of its contents (if it's consumable) or null item otherwise.
         * WARNING: consumable does not necessarily guarantee the comestible type.
         */
        item &get_consumable_from( item &it ) const;

        /** Get calorie & vitamin contents for a comestible, taking into
         * account character traits */
        /** Get range of possible nutrient content, for a particular recipe,
         * depending on choice of ingredients */
        std::pair<nutrients, nutrients> compute_nutrient_range(
            const item &, const recipe_id &,
            const cata::flat_set<flag_id> &extra_flags = {} ) const;
        /** Same, but across arbitrary recipes */
        std::pair<nutrients, nutrients> compute_nutrient_range(
            const itype_id &, const cata::flat_set<flag_id> &extra_flags = {} ) const;
        /** Returns allergy type or MORALE_NULL if not allergic for this character */
        morale_type allergy_type( const item &food ) const;
        nutrients compute_effective_nutrients( const item & ) const;
        /** Returns true if the character is wearing something on the entered body part */
        bool wearing_something_on( const bodypart_id &bp ) const;
        /** Returns true if the character is wearing something occupying the helmet slot */
        bool is_wearing_helmet() const;
        /** Returns the total encumbrance of all SKINTIGHT and HELMET_COMPAT items covering
         *  the head */
        int head_cloth_encumbrance() const;
        /** Same as footwear factor, but for arms */
        double armwear_factor() const;
        /** Returns 1 if the player is wearing an item of that count on one foot, 2 if on both,
         *  and zero if on neither */
        int shoe_type_count( const itype_id &it ) const;
        /** Returns 1 if the player is wearing footwear on both feet, .5 if on one,
         *  and 0 if on neither */
        double footwear_factor() const;
        /** Returns true if the player is wearing something on their feet that is not SKINTIGHT */
        bool is_wearing_shoes( const side &which_side = side::BOTH ) const;

        /** Swap side on which item is worn; returns false on fail. If interactive is false, don't alert player or drain moves */
        bool change_side( item &it, bool interactive = true );
        bool change_side( item_location &loc, bool interactive = true );

        bool get_check_encumbrance() const {
            return check_encumbrance;
        }
        void set_check_encumbrance( bool new_check ) {
            check_encumbrance = new_check;
        }
        /** Ticks down morale counters and removes them */
        void update_morale();
        /** Ensures persistent morale effects are up-to-date */
        void apply_persistent_morale();
        // the morale penalty for hoarders
        void hoarder_morale_penalty();
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
        /**
        * Returns items that can be used to craft with. Always includes character inventory.
        * @param src_pos Character position.
        * @param radius Radius from src_pos. -1 to return items in character inventory only.
        * @param clear_path True to select only items within view. False to select all within the radius.
        * @returns Craftable inventory items found.
        * */
        const inventory &crafting_inventory( const tripoint &src_pos = tripoint_zero,
                                             int radius = PICKUP_RANGE, bool clear_path = true );
        void invalidate_crafting_inventory();

        /** Returns a value from 1.0 to 11.0 that acts as a multiplier
         * for the time taken to perform tasks that require detail vision,
         * above 4.0 means these activities cannot be performed.
         * takes pos as a parameter so that remote spots can be judged
         * if they will potentially have enough light when player gets there */
        float fine_detail_vision_mod( const tripoint &p = tripoint_zero ) const;

        // ---- CRAFTING ----
        void make_craft_with_command( const recipe_id &id_to_make, int batch_size, bool is_long,
                                      const cata::optional<tripoint> &loc );
        pimpl<craft_command> last_craft;

        recipe_id lastrecipe;
        int last_batch;
        itype_id lastconsumed;        //used in crafting.cpp and construction.cpp

        // Checks crafting inventory for books providing the requested recipe.
        // Then checks nearby NPCs who could provide it too.
        // Returns -1 to indicate recipe not found, otherwise difficulty to learn.
        int has_recipe( const recipe *r, const inventory &crafting_inv,
                        const std::vector<npc *> &helpers ) const;
        bool knows_recipe( const recipe *rec ) const;
        void learn_recipe( const recipe *rec );
        int exceeds_recipe_requirements( const recipe &rec ) const;
        bool has_recipe_requirements( const recipe &rec ) const;
        bool can_decomp_learn( const recipe &rec ) const;

        bool studied_all_recipes( const itype &book ) const;

        /** Returns all known recipes. */
        const recipe_subset &get_learned_recipes() const;
        /** Returns all recipes that are known from the books (either in inventory or nearby). */
        recipe_subset get_recipes_from_books( const inventory &crafting_inv ) const;
        /**
          * Returns all available recipes (from books and npc companions)
          * @param crafting_inv Current available items to craft
          * @param helpers List of NPCs that could help with crafting.
          */
        recipe_subset get_available_recipes( const inventory &crafting_inv,
                                             const std::vector<npc *> *helpers = nullptr ) const;
        /**
          * Returns the set of book types in crafting_inv that provide the
          * given recipe.
          * @param crafting_inv Current available items that may contain readable books
          * @param r Recipe to search for in the available books
          */
        std::set<itype_id> get_books_for_recipe( const inventory &crafting_inv,
                const recipe *r ) const;

        // crafting.cpp
        float morale_crafting_speed_multiplier( const recipe &rec ) const;
        float lighting_craft_speed_multiplier( const recipe &rec ) const;
        float crafting_speed_multiplier( const recipe &rec, bool in_progress = false ) const;
        /** For use with in progress crafts */
        float crafting_speed_multiplier( const item &craft, const cata::optional<tripoint> &loc ) const;
        int available_assistant_count( const recipe &rec ) const;
        /**
         * Time to craft not including speed multiplier
         */
        int64_t base_time_to_craft( const recipe &rec, int batch_size = 1 ) const;
        /**
         * Expected time to craft a recipe, with assumption that multipliers stay constant.
         */
        int64_t expected_time_to_craft( const recipe &rec, int batch_size = 1,
                                        bool in_progress = false ) const;
        std::vector<const item *> get_eligible_containers_for_crafting() const;
        bool check_eligible_containers_for_crafting( const recipe &rec, int batch_size = 1 ) const;
        bool can_make( const recipe *r, int batch_size = 1 );  // have components?
        /**
         * Returns true if the player can start crafting the recipe with the given batch size
         * The player is not required to have enough tool charges to finish crafting, only to
         * complete the first step (total / 20 + total % 20 charges)
         */
        bool can_start_craft( const recipe *rec, recipe_filter_flags, int batch_size = 1 );
        bool making_would_work( const recipe_id &id_to_make, int batch_size );

        /**
         * Start various types of crafts
         * @param loc the location of the workbench. cata::nullopt indicates crafting from inventory.
         */
        void craft( const cata::optional<tripoint> &loc = cata::nullopt );
        void recraft( const cata::optional<tripoint> &loc = cata::nullopt );
        void long_craft( const cata::optional<tripoint> &loc = cata::nullopt );
        void make_craft( const recipe_id &id, int batch_size,
                         const cata::optional<tripoint> &loc = cata::nullopt );
        void make_all_craft( const recipe_id &id, int batch_size,
                             const cata::optional<tripoint> &loc );
        /** consume components and create an active, in progress craft containing them */
        void start_craft( craft_command &command, const cata::optional<tripoint> &loc );
        /**
         * Calculate a value representing the success of the player at crafting the given recipe,
         * taking player skill, recipe difficulty, npc helpers, and player mutations into account.
         * @param making the recipe for which to calculate
         * @return a value >= 0.0 with >= 1.0 representing unequivocal success
         */
        double crafting_success_roll( const recipe &making ) const;
        void complete_craft( item &craft, const cata::optional<tripoint> &loc );
        /**
         * Check if the player meets the requirements to continue the in progress craft and if
         * unable to continue print messages explaining the reason.
         * If the craft is missing components due to messing up, prompt to consume new ones to
         * allow the craft to be continued.
         * @param craft the currently in progress craft
         * @return if the craft can be continued
         */
        bool can_continue_craft( item &craft );
        bool can_continue_craft( item &craft, const requirement_data &continue_reqs );
        /** Returns nearby NPCs ready and willing to help with crafting. */
        std::vector<npc *> get_crafting_helpers() const;
        int get_num_crafting_helpers( int max ) const;
        /**
         * Handle skill gain for player and followers during crafting
         * @param craft the currently in progress craft
         * @param num_practice_ticks to trigger.  This is used to apply
         * multiple steps of incremental skill gain simultaneously if needed.
         */
        void craft_skill_gain( const item &craft, const int &num_practice_ticks );
        /**
         * Handle proficiency practice for player and followers while crafting
         * @param craft - the in progress craft
         * @param time - the amount of time since the last practice tick
         */
        void craft_proficiency_gain( const item &craft, const time_duration &time );
        /**
         * Check if the player can disassemble an item using the current crafting inventory
         * @param obj Object to check for disassembly
         * @param inv current crafting inventory
         */
        ret_val<bool> can_disassemble( const item &obj, const read_only_visitable &inv ) const;
        item_location create_in_progress_disassembly( item_location target );

        bool disassemble();
        bool disassemble( item_location target, bool interactive = true );
        void disassemble_all( bool one_pass ); // Disassemble all items on the tile
        void complete_disassemble( item_location target );
        void complete_disassemble( item_location &target, const recipe &dis );

        const requirement_data *select_requirements(
            const std::vector<const requirement_data *> &, int batch, const read_only_visitable &,
            const std::function<bool( const item & )> &filter ) const;
        comp_selection<item_comp>
        select_item_component( const std::vector<item_comp> &components,
                               int batch, read_only_visitable &map_inv, bool can_cancel = false,
                               const std::function<bool( const item & )> &filter = return_true<item>, bool player_inv = true );
        std::list<item> consume_items( const comp_selection<item_comp> &is, int batch,
                                       const std::function<bool( const item & )> &filter = return_true<item> );
        std::list<item> consume_items( map &m, const comp_selection<item_comp> &is, int batch,
                                       const std::function<bool( const item & )> &filter = return_true<item>,
                                       const tripoint &origin = tripoint_zero, int radius = PICKUP_RANGE );
        std::list<item> consume_items( const std::vector<item_comp> &components, int batch = 1,
                                       const std::function<bool( const item & )> &filter = return_true<item> );
        bool consume_software_container( const itype_id &software_id );
        comp_selection<tool_comp>
        select_tool_component( const std::vector<tool_comp> &tools, int batch, read_only_visitable &map_inv,
                               bool can_cancel = false, bool player_inv = true,
        const std::function<int( int )> &charges_required_modifier = []( int i ) {
            return i;
        } );
        /** Consume tools for the next multiplier * 5% progress of the craft */
        bool craft_consume_tools( item &craft, int multiplier, bool start_craft );
        void consume_tools( const comp_selection<tool_comp> &tool, int batch );
        void consume_tools( map &m, const comp_selection<tool_comp> &tool, int batch,
                            const tripoint &origin = tripoint_zero, int radius = PICKUP_RANGE,
                            basecamp *bcp = nullptr );
        void consume_tools( const std::vector<tool_comp> &tools, int batch = 1 );

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
        int heartrate_bpm() const;
        std::vector<std::string> short_description_parts() const;
        std::string short_description() const;
        // Checks whether a player can hear a sound at a given volume and location.
        bool can_hear( const tripoint &source, int volume ) const;
        // Returns a multiplier indicating the keenness of a player's hearing.
        float hearing_ability() const;

        using trap_map = std::map<tripoint, std::string>;
        // Use @ref trap::can_see to check whether a character knows about a
        // specific trap - it will consider visibile and known traps.
        bool knows_trap( const tripoint &pos ) const;
        void add_known_trap( const tripoint &pos, const trap &t );
        /** Define color for displaying the body temperature */
        nc_color bodytemp_color( const bodypart_id &bp ) const;

        // see Creature::sees
        bool sees( const tripoint &t, bool is_player = false, int range_mod = 0 ) const override;
        // see Creature::sees
        bool sees( const Creature &critter ) const override;
        Attitude attitude_to( const Creature &other ) const override;
        virtual npc_attitude get_attitude() const;

        // used in debugging all health
        int get_lowest_hp() const;
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
        time_duration get_consume_time( const item &it );

        // For display purposes mainly, how far we are from the next level of weariness
        std::pair<int, int> weariness_transition_progress() const;
        int weariness_level() const;
        int weary_threshold() const;
        int weariness() const;
        float activity_level() const;
        float exertion_adjusted_move_multiplier( float level = -1.0f ) const;
        void try_reduce_weariness( float exertion );
        float maximum_exertion_level() const;
        std::string debug_weary_info() const;
        // returns empty because this is avatar specific
        void add_pain_msg( int, const bodypart_id & ) const {}
        /** Returns the modifier value used for vomiting effects. */
        double vomit_mod();
        /** Checked each turn during "lying_down", returns true if the player falls asleep */
        bool can_sleep();
        /** Rate point's ability to serve as a bed. Takes all mutations, fatigue and stimulants into account. */
        int sleep_spot( const tripoint &p ) const;
        /** Processes human-specific effects of effects before calling Creature::process_effects(). */
        void process_effects() override;
        /** Handles the still hard-coded effects. */
        void hardcoded_effects( effect &it );

        // inherited from visitable
        bool has_quality( const quality_id &qual, int level = 1, int qty = 1 ) const override;
        int max_quality( const quality_id &qual ) const override;
        VisitResponse visit_items( const std::function<VisitResponse( item *, item * )> &func ) const
        override;
        std::list<item> remove_items_with( const std::function<bool( const item & )> &filter,
                                           int count = INT_MAX ) override;
        int charges_of( const itype_id &what, int limit = INT_MAX,
                        const std::function<bool( const item & )> &filter = return_true<item>,
                        const std::function<void( int )> &visitor = nullptr ) const override;
        int amount_of( const itype_id &what, bool pseudo = true,
                       int limit = INT_MAX,
                       const std::function<bool( const item & )> &filter = return_true<item> ) const override;

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
        int str_bonus = 0;
        int dex_bonus = 0;
        int per_bonus = 0;
        int int_bonus = 0;

        /** How healthy the character is. */
        int healthy = 0;
        int healthy_mod = 0;

        weariness_tracker weary;
        // Our bmr at no activity level
        int base_bmr() const;

        /** age in years at character creation */
        int init_age = 25;
        /**height at character creation*/
        int init_height = 175;
        /** Size class of character. */
        creature_size size_class = creature_size::medium;

        // the player's activity level for metabolism calculations
        float attempted_activity_level = NO_EXERCISE;
        // Display purposes only - the highest activity this turn and last
        float act_cursor = NO_EXERCISE;
        float last_act = NO_EXERCISE;
        time_point act_turn = calendar::turn_zero;

        trap_map known_traps;
        mutable std::map<std::string, double> cached_info;
        bool bio_soporific_powered_at_last_sleep_check = false;
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
        /**
         * The amount of weight the Character is carrying.
         * If it is nullopt, needs to be recalculated
         */
        mutable cata::optional<units::mass> cached_weight_carried = cata::nullopt;

        void store( JsonOut &json ) const;
        void load( const JsonObject &data );

        // --------------- Values ---------------
        pimpl<SkillLevelMap> _skills;

        pimpl<proficiency_set> _proficiencies;

        // Cached vision values.
        std::bitset<NUM_VISION_MODES> vision_mode_cache;
        int sight_max = 0;

        /// turn the character expired, if calendar::before_time_starts it has not been set yet.
        /// @todo change into an optional<time_point>
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

        move_mode_id move_mode;
        /** Current deficiency/excess quantity for each vitamin */
        std::map<vitamin_id, int> vitamin_levels;

        pimpl<player_morale> morale;
        /** Processes human-specific effects of an effect. */
        void process_one_effect( effect &it, bool is_new ) override;

    public:
        /**
         * Map body parts to their total exposure, from 0.0 (fully covered) to 1.0 (buck naked).
         * Clothing layers are multiplied, ex. two layers of 50% coverage will leave only 25% exposed.
         * Used to determine suffering effects of albinism and solar sensitivity.
         */
        std::map<bodypart_id, float> bodypart_exposure();
    private:
        /** suffer() subcalls */
        void suffer_water_damage( const trait_id &mut_id );
        void suffer_mutation_power( const trait_id &mut_id );
        void suffer_while_underwater();
        void suffer_from_addictions();
        void suffer_while_awake( int current_stim );
        void suffer_from_chemimbalance();
        void suffer_from_schizophrenia();
        void suffer_from_asthma( int current_stim );
        void suffer_from_pain();
        void suffer_in_sunlight();
        void suffer_from_sunburn();
        void suffer_from_other_mutations();
        void suffer_from_item_dropping();
        void suffer_from_radiation();
        void suffer_from_bad_bionics();
        void suffer_from_stimulants( int current_stim );
        void suffer_from_exertion();
        void suffer_without_sleep( int sleep_deprivation );
        void suffer_from_tourniquet();
        /**
         * Check whether the other creature is in range and can be seen by this creature.
         * @param critter Creature to check for visibility
         * @param range The maximal distance (@ref rl_dist), creatures at this distance or less
         * are included.
         */
        bool is_visible_in_range( const Creature &critter, int range ) const;

        struct auto_toggle_bionic_result;

        /**
         * Automatically turn bionic on or off according to remaining fuel and
         * user settings, and return info of the first burnable fuel.
         */
        auto_toggle_bionic_result auto_toggle_bionic( int b, bool start );
        /**
         *Convert fuel to bionic power
         */
        void burn_fuel( int b, const auto_toggle_bionic_result &result );

        // a cache of all active enchantment values.
        // is recalculated every turn in Character::recalculate_enchantment_cache
        pimpl<enchantment> enchantment_cache;
        player_activity destination_activity;
        /// A unique ID number, assigned by the game class. Values should never be reused.
        character_id id;

        units::energy power_level;
        units::energy max_power_level;

        // Our weariness level last turn, so we know when we transition
        int old_weary_level = 0;

        /// @brief Needs (hunger, starvation, thirst, fatigue, etc.)
        // Stored calories is a value in 'calories' - 1/1000s of kcals (or Calories)
        int stored_calories;
        int healthy_calories;

        int hunger;
        int thirst;
        int stamina;

        int fatigue;
        int sleep_deprivation;
        bool check_encumbrance = true;
        bool cache_inventory_is_valid = false;

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
        pimpl<inventory> cached_crafting_inventory;

    protected:
        /** Subset of learned recipes. Needs to be mutable for lazy initialization. */
        mutable pimpl<recipe_subset> learned_recipes;

        /** Stamp of skills. @ref learned_recipes are valid only with this set of skills. */
        mutable decltype( _skills ) valid_autolearn_skills;

        /** Amount of time the player has spent in each overmap tile. */
        std::unordered_map<point_abs_omt, time_duration> overmap_time;

    public:
        time_point next_climate_control_check;
        bool last_climate_control_ret;
};

Character &get_player_character();

template<>
struct enum_traits<character_stat> {
    static constexpr character_stat last = character_stat::DUMMY_STAT;
};
/// Get translated name of a stat
std::string get_stat_name( character_stat Stat );
#endif // CATA_SRC_CHARACTER_H
