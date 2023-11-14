#pragma once
#ifndef CATA_SRC_CHARACTER_H
#define CATA_SRC_CHARACTER_H

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
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "activity_tracker.h"
#include "activity_type.h"
#include "addiction.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character_attire.h"
#include "character_id.h"
#include "city.h"
#include "coordinates.h"
#include "craft_command.h"
#include "creature.h"
#include "damage.h"
#include "debug.h"
#include "enums.h"
#include "flat_set.h"
#include "game_constants.h"
#include "item.h"
#include "item_location.h"
#include "item_pocket.h"
#include "magic_enchantment.h"
#include "memory_fast.h"
#include "pimpl.h"
#include "player_activity.h"
#include "point.h"
#include "ranged.h"
#include "recipe.h"
#include "ret_val.h"
#include "stomach.h"
#include "string_formatter.h"
#include "type_id.h"
#include "units_fwd.h"
#include "visitable.h"
#include "weakpoint.h"
#include "weighted_list.h"

class Character;
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
class player_morale;
class proficiency_set;
class recipe_subset;
class spell;
class ui_adaptor;
class vpart_reference;
class vehicle;
struct bionic;
struct construction;
struct dealt_projectile_attack;
struct display_proficiency;
/// @brief Item slot used to apply modifications from food and meds
struct islot_comestible;
struct item_comp;
struct itype;
struct mutation_branch;
struct mutation_variant;
struct needs_rates;
struct pathfinding_settings;
struct points_left;
struct requirement_data;
struct tool_comp;
struct trait_and_var;
struct trap;
struct w_point;
template <typename E> struct enum_traits;

enum npc_attitude : int;
enum action_id : int;
enum class steed_type : int;
enum class proficiency_bonus_type : int;

using drop_location = std::pair<item_location, int>;
using drop_locations = std::list<drop_location>;

using bionic_uid = unsigned int;

constexpr int MAX_CLAIRVOYANCE = 40;
// kcal in a kilogram of fat, used to convert stored kcal into body weight. 3500kcal/lb * 2.20462lb/kg = 7716.17
constexpr float KCAL_PER_KG = 3500 * 2.20462;

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

enum crush_tool_type {
    CRUSH_EMPTY_HANDS,
    CRUSH_HAMMER,
    CRUSH_DRILL_OR_HAMMER_AND_SCREW,
    CRUSH_NO_TOOL
};

struct queued_eoc {
    public:
        effect_on_condition_id eoc;
        time_point time;
        std::unordered_map<std::string, std::string> context;
};

struct eoc_compare {
    bool operator()( const queued_eoc &lhs, const queued_eoc &rhs ) const {
        return lhs.time > rhs.time;
    }
};

struct queued_eocs {
    using storage_iter = std::list<queued_eoc>::iterator;

    struct eoc_compare : ::eoc_compare {
        bool operator()( const storage_iter &lhs, const storage_iter &rhs ) const {
            return ::eoc_compare::operator()( *lhs, *rhs );
        }
    };
    std::priority_queue<storage_iter, std::vector<storage_iter>, eoc_compare> queue;
    std::list<queued_eoc> list;

    queued_eocs() = default;

    queued_eocs( const queued_eocs &rhs ) {
        list = rhs.list;
        for( auto it = list.begin(), end = list.end(); it != end; ++it ) {
            queue.emplace( it );
        }
    };
    queued_eocs( queued_eocs &&rhs ) noexcept {
        queue.swap( rhs.queue );
        list.swap( rhs.list );
    }

    queued_eocs &operator=( const queued_eocs &rhs ) {
        list = rhs.list;
        // Why doesn't std::priority_queue have a clear() function.
        queue = {};
        for( auto it = list.begin(), end = list.end(); it != end; ++it ) {
            queue.emplace( it );
        }
        return *this;
    }
    queued_eocs &operator=( queued_eocs &&rhs ) noexcept {
        queue.swap( rhs.queue );
        list.swap( rhs.list );
        return *this;
    }

    /* std::priority_queue compatibility layer where performance is less relevant */

    bool empty() const {
        return queue.empty();
    }

    const queued_eoc &top() const {
        return *queue.top();
    }

    void push( const queued_eoc &eoc ) {
        auto it = list.emplace( list.end(), eoc );
        queue.push( it );
    }

    void pop() {
        auto it = queue.top();
        queue.pop();
        list.erase( it );
    }
};

struct aim_type {
    std::string name;
    std::string action;
    std::string help;
    bool has_threshold;
    int threshold;
    bool operator==( const aim_type &other ) const {
        return has_threshold == other.has_threshold &&
               threshold == other.threshold &&
               name == other.name &&
               action == other.action &&
               help == other.help;
    }
};

struct parallax_cache {
    int parallax_with_zoom;
    int parallax_without_zoom;
};

struct aim_mods_cache {
    float aim_speed_skill_mod;
    float aim_speed_dex_mod;
    float aim_speed_mod;
    int limit;
    double aim_factor_from_volume;
    double aim_factor_from_length;
    std::optional<std::reference_wrapper<parallax_cache>> parallaxes;
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
    void deserialize( const JsonObject &jo );
};

struct stat_mod {
    int strength = 0;
    int dexterity = 0;
    int intelligence = 0;
    int perception = 0;

    int speed = 0;
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

enum class customize_appearance_choice : int {
    EYES, // customize eye color
    HAIR, // customize hair
    HAIR_F, // customize facial hair
    SKIN  // customize skin color
};

enum class book_mastery {
    CANT_DETERMINE, // book not yet identified, so you don't know yet
    CANT_UNDERSTAND, // does not have enough skill to read
    LEARNING,
    MASTERED // can no longer increase skill by reading
};

enum class read_condition_result {
    SUCCESS = 0,
    NOT_BOOK = 1 << 0,
    CANT_UNDERSTAND = 1 << 1,
    MASTERED = 1 << 2,
    DRIVING = 1 << 3,
    ILLITERATE = 1 << 4,
    NEED_GLASSES = 1 << 5,
    TOO_DARK = 1 << 6,
    MORALE_LOW = 1 << 7,
    BLIND = 1 << 8
};

template<>
struct enum_traits<read_condition_result> {
    static constexpr bool is_flag_enum = true;
};

/** @relates ret_val */
template<>
struct ret_val<edible_rating>::default_success : public
    std::integral_constant<edible_rating, EDIBLE> {};

/** @relates ret_val */
template<>
struct ret_val<edible_rating>::default_failure : public
    std::integral_constant<edible_rating, INEDIBLE> {};

/** @relates ret_val */
template<>
struct ret_val<crush_tool_type>::default_success : public
    std::integral_constant<crush_tool_type, CRUSH_EMPTY_HANDS> {};

/** @relates ret_val */
template<>
struct ret_val<crush_tool_type>::default_failure : public
    std::integral_constant<crush_tool_type, CRUSH_NO_TOOL> {};

struct needs_rates {
    float thirst = 0.0f;
    float hunger = 0.0f;
    float fatigue = 0.0f;
    float recovery = 0.0f;
    float kcal = 0.0f;
};

struct pocket_data_with_parent {
    const item_pocket *pocket_ptr;
    item_location parent;
    int nested_level;
};

struct speed_bonus_effect {
    std::string description;
    int bonus = 0;
};

struct run_cost_effect {
    std::string description;
    float times = 1.0;
    float plus = 0;
};

class Character : public Creature, public visitable
{
    public:
        Character( const Character & ) = delete;
        Character &operator=( const Character & ) = delete;
        ~Character() override;

        // initialize avatar and avatar mocks
        void initialize( bool learn_recipes = true );

        Character *as_character() override {
            return this;
        }
        const Character *as_character() const override {
            return this;
        }
        bool is_npc() const override {
            return false;    // Overloaded for NPCs in npc.h
        }
        // populate variables, inventory items, and misc from json object
        virtual void deserialize( const JsonObject &jsobj ) = 0;

        // by default save all contained info
        virtual void serialize( JsonOut &jsout ) const = 0;

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

        int kill_xp = 0;
        // Level-up points spent on Stats through Kills
        int spent_upgrade_points = 0;

        const profession *prof;
        std::set<const profession *> hobbies;

        // Relative direction of a grab, add to posx, posy to get the coordinates of the grabbed thing.
        tripoint grab_point;

        std::optional<city> starting_city;
        std::optional<point_abs_om> world_origin;
        bool random_start_location = true;
        start_location_id start_location;

        bool manual_examine = false;
        int volume = 0;
        // The prevalence of getter, setter, and mutator functions here is partially
        // a result of the slow, piece-wise migration of the player class upwards into
        // the character class. As enough logic is moved upwards to fully separate
        // utility upwards out of the player class, as many of these as possible should
        // be eliminated to allow for proper code separation. (Note: Not "all", many").
        /** Getters for stats exclusive to characters */
        int get_str() const;
        int get_dex() const;
        int get_per() const;
        int get_int() const;

        int get_str_base() const;
        int get_dex_base() const;
        int get_per_base() const;
        int get_int_base() const;

        int get_str_bonus() const;
        int get_dex_bonus() const;
        int get_per_bonus() const;
        int get_int_bonus() const;

    private:
        /** Modifiers to character speed, with descriptions */
        std::vector<speed_bonus_effect> speed_bonus_effects;

        int dodges_left;

    public:
        std::vector<speed_bonus_effect> get_speed_bonus_effects() const;
        int get_speed() const override;
        int get_enchantment_speed_bonus() const;
        // Strength modified by limb lifting score
        int get_arm_str() const;
        // Defines distance from which CAMOUFLAGE mobs are visible
        int get_eff_per() const override;

        // Penalty modifiers applied for ranged attacks due to low stats
        int ranged_dex_mod() const;
        int ranged_per_mod() const;

        /** Setters for stats exclusive to characters */
        void set_str_bonus( int nstr );
        void set_dex_bonus( int ndex );
        void set_per_bonus( int nper );
        void set_int_bonus( int nint );
        void mod_str_bonus( int nstr );
        void mod_dex_bonus( int ndex );
        void mod_per_bonus( int nper );
        void mod_int_bonus( int nint );

        /** Setters for stats shared with other creatures */
        using Creature::mod_speed_bonus;
        void mod_speed_bonus( int nspeed, const std::string &desc );

        // Prints message(s) about current health
        void print_health() const;

        /** Getters for health values exclusive to characters */
        int get_lifestyle() const;
        int get_daily_health() const;
        int get_health_tally() const;

        /** Modifiers for health values exclusive to characters */
        void mod_livestyle( int nhealthy );
        void mod_daily_health( int nhealthy_mod, int cap );
        void mod_health_tally( int mod );

        /** Setters for health values exclusive to characters */
        void set_lifestyle( int nhealthy );
        void set_daily_health( int nhealthy_mod );

        /** Getter for need values exclusive to characters */
        int get_stored_kcal() const;
        int get_healthy_kcal() const;
        // Returns stored kcals as a proportion of "healthy" kcals (1.0 == healthy)
        float get_kcal_percent() const;
        int kcal_speed_penalty() const;
        int get_hunger() const;
        int get_starvation() const;
        virtual int get_thirst() const;
        virtual int get_instant_thirst() const;

        time_duration get_daily_sleep() const;
        void mod_daily_sleep( time_duration mod );
        void reset_daily_sleep();
        time_duration get_continuous_sleep() const;
        void mod_continuous_sleep( time_duration mod );
        void reset_continuous_sleep();

        int get_fatigue() const;
        int get_sleep_deprivation() const;

        /** Modifiers for need values exclusive to characters */
        void mod_stored_kcal( int nkcal, bool ignore_weariness = false );
        void mod_hunger( int nhunger );
        void mod_thirst( int nthirst );
        void mod_fatigue( int nfatigue );
        void mod_sleep_deprivation( int nsleep_deprivation );

        /** Setters for need values exclusive to characters */
        void set_stored_kcal( int kcal );
        void set_hunger( int nhunger );
        void set_thirst( int nthirst );
        void set_fatigue( int nfatigue );
        void set_fatigue( fatigue_levels nfatigue );
        void set_sleep_deprivation( int nsleep_deprivation );

    protected:

        // These accept values in calories, 1/1000s of kcals (or Calories)
        void mod_stored_calories( int ncal, bool ignore_weariness = false );
        void set_stored_calories( int cal );

    public:

        void gravity_check();

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
        virtual std::string name_and_maybe_activity() const;
        /** Returns the name of the player's outer layer, e.g. "armor plates" */
        std::string skin_name() const override;

        //message related stuff
        using Creature::add_msg_if_player;
        void add_msg_if_player( const std::string &msg ) const override;
        void add_msg_if_player( const game_message_params &params, const std::string &msg ) const override;
        using Creature::add_msg_debug_if_player;
        void add_msg_debug_if_player( debugmode::debug_filter type,
                                      const std::string &msg ) const override;
        using Creature::add_msg_player_or_npc;
        void add_msg_player_or_npc( const std::string &player_msg,
                                    const std::string &npc_str ) const override;
        void add_msg_player_or_npc( const game_message_params &params, const std::string &player_msg,
                                    const std::string &npc_msg ) const override;
        using Creature::add_msg_debug_player_or_npc;
        void add_msg_debug_player_or_npc( debugmode::debug_filter type, const std::string &player_msg,
                                          const std::string &npc_msg ) const override;
        using Creature::add_msg_player_or_say;
        void add_msg_player_or_say( const std::string &player_msg,
                                    const std::string &npc_speech ) const override;
        void add_msg_player_or_say( const game_message_params &params, const std::string &player_msg,
                                    const std::string &npc_speech ) const override;

        /* returns the character's faction */
        virtual faction *get_faction() const {
            return nullptr;
        }
        void set_fac_id( const std::string &my_fac_id );
        virtual bool is_ally( const Character &p ) const = 0;
        virtual bool is_obeying( const Character &p ) const = 0;

        // Has item with mission_id
        bool has_mission_item( int mission_id ) const;

        /* Adjusts provided sight dispersion to account for player stats */
        int effective_dispersion( int dispersion, bool zoom = false ) const;

        int get_character_parallax( bool zoom = false ) const;

        /* Accessors for aspects of aim speed. */
        std::vector<aim_type> get_aim_types( const item &gun ) const;
        int point_shooting_limit( const item &gun ) const;
        double fastest_aiming_method_speed( const item &gun, double recoil,
                                            Target_attributes target_attributes = Target_attributes(),
                                            std::optional<std::reference_wrapper<parallax_cache>> parallax_cache = std::nullopt ) const;
        int most_accurate_aiming_method_limit( const item &gun ) const;
        double aim_factor_from_volume( const item &gun ) const;
        double aim_factor_from_length( const item &gun ) const;
        aim_mods_cache gen_aim_mods_cache( const item &gun )const;

        // Get the value of the specified character modifier.
        // (some modifiers require a skill_id, ex: aim_speed_skill_mod)
        float get_modifier( const character_modifier_id &mod,
                            const skill_id &skill = skill_id::NULL_ID() ) const;

        /* Gun stuff */
        /**
         * Check whether the player has a gun that uses the given type of ammo.
         */
        bool has_gun_for_ammo( const ammotype &at ) const;
        bool has_magazine_for_ammo( const ammotype &at ) const;

        /* Calculate aim improvement per move spent aiming at a given @ref recoil
        * Use a struct to avoid repeatedly calculate some modifiers that are actually persistent for aiming UI drawing.
        */
        double aim_per_move( const item &gun, double recoil,
                             Target_attributes target_attributes = Target_attributes(),
                             std::optional<std::reference_wrapper<const aim_mods_cache>> aim_cache = std::nullopt ) const;

        int get_dodges_left() const;
        void set_dodges_left( int dodges );
        void mod_dodges_left( int mod );
        void consume_dodge_attempts();
        ret_val<void> can_try_doge( bool ignore_dodges_left = false ) const;

        float get_stamina_dodge_modifier() const;

        /** Called after the player has successfully dodged an attack */
        void on_dodge( Creature *source, float difficulty ) override;
        /** Called after the player has tryed to dodge an attack */
        void on_try_dodge() override;

        /** Combat getters */
        float get_dodge_base() const override;
        /** Returns the player's dodge_roll to be compared against an aggressor's hit_roll() */
        float dodge_roll() const override;
        /** Returns Creature::get_dodge() modified by any Character effects */
        float get_dodge() const override;
        /** in this case spell resistance is just the spellcraft skill for characters. */
        int get_spell_resist() const override;
        /** Handles the uncanny dodge bionic and effects, returns true if the player successfully dodges */
        bool uncanny_dodge() override;
        float get_hit_base() const override;

        /** Returns the player's sight range */
        int sight_range( float light_level ) const override;
        /** Returns the player maximum vision range factoring in mutations, diseases, and other effects */
        int  unimpaired_range() const;
        /** Returns true if overmap tile is within player line-of-sight */
        bool overmap_los( const tripoint_abs_omt &omt, int sight_points ) const;
        /** Returns the distance the player can see on the overmap */
        int  overmap_sight_range( float light_level ) const;
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
        /** Returns true if the player is knocked over, has broken legs or is lying down */
        bool is_on_ground() const override;
        /** Returns the player's movecost for swimming across water tiles. NOT SPEED! */
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
        /**
         * Check player capable of taking off an item.
         * @param it Thing to be taken off
         */
        ret_val<void> can_takeoff( const item &it, const std::list<item> *res = nullptr );

        /** @return Odds for success (pair.first) and gunmod damage (pair.second) */
        std::pair<int, int> gunmod_installation_odds( const item_location &gun, const item &mod ) const;
        /** Calculates the various speed bonuses we will get from mutations, etc. */
        void recalc_speed_bonus();
        void set_underwater( bool );
        bool is_hallucination() const override;

        // true if the character produces electrical radiation
        bool is_electrical() const override;
        // true if the character is from the nether
        bool is_nether() const override;
        // true if the character has a sapient mind
        bool has_mind() const override;
        /** Returns the penalty to speed from thirst */
        static int thirst_speed_penalty( int thirst );
        /** Returns the effect of pain on stats */
        stat_mod get_pain_penalty() const;
        /** returns players strength adjusted by any traits that affect strength during lifting jobs */
        int get_lift_str() const;
        /** Takes off an item, returning false on fail. The taken off item is processed in the interact */
        bool takeoff( item_location loc, std::list<item> *res = nullptr );
        bool takeoff( int pos );

        /** Handles health fluctuations over time */
        virtual void update_health();
        /** Updates all "biology" by one turn. Should be called once every turn. */
        void update_body();
        /** Updates all "biology" as if time between `from` and `to` passed. */
        void update_body( const time_point &from, const time_point &to );
        /** Updates the stomach to give accurate hunger messages */
        void update_stomach( const time_point &from, const time_point &to );
        /** Updates the mutations from enchantments */
        void update_enchantment_mutations();
        /** Returns true if character needs food, false if character is an NPC with NO_NPC_FOOD set */
        bool needs_food() const;
        /** Increases hunger, thirst, fatigue and stimulants wearing off. `rate_multiplier` is for retroactive updates. */
        void update_needs( int rate_multiplier );
        needs_rates calc_needs_rates() const;
        void calc_sleep_recovery_rate( needs_rates &rates ) const;
        /** Kills the player if too hungry, stimmed up etc., forces tired player to sleep and prints warnings. */
        void check_needs_extremes();
        /** Handles the chance to be infected by random diseases */
        void get_sick( bool is_flu = false );
        /** Returns if the player has hibernation mutation and is asleep and well fed */
        bool is_hibernating() const;
        /** Maintains body temperature */
        void update_bodytemp();
        void update_frostbite( const bodypart_id &bp, int FBwindPower,
                               const std::map<bodypart_id, int> &warmth_per_bp );
        /** Equalizes heat between body parts */
        void temp_equalizer( const bodypart_id &bp1, const bodypart_id &bp2 );

        struct comfort_response_t {
            comfort_level level = comfort_level::neutral;
            const item *aid = nullptr;
        };
        /** Rate point's ability to serve as a bed. Only takes certain mutations into account, and not fatigue nor stimulants. */
        comfort_response_t base_comfort_value( const tripoint &p ) const;

        /** Returns focus equilibrium cap due to fatigue **/
        int focus_equilibrium_fatigue_cap( int equilibrium ) const;
        /** Uses morale and other factors to return the character's focus target goto value */
        int calc_focus_equilibrium( bool ignore_pain = false ) const;
        /** Calculates actual focus gain/loss value from focus equilibrium*/
        int calc_focus_change() const;
        /** Uses calc_focus_change to update the character's current focus */
        void update_mental_focus();

        /** Resets the value of all bonus fields to 0. */
        void reset_bonuses() override;
        /** Resets stats, and applies effects in an idempotent manner */
        void reset_stats() override;
        /** Handles stat and bonus reset. */
        void reset() override;

        /** Returns ENC provided by armor, etc. */
        int encumb( const bodypart_id &bp ) const;
        int avg_encumb_of_limb_type( body_part_type::type part_type ) const;

        /** Returns body weight plus weight of inventory and worn/wielded items */
        units::mass get_weight() const override;

        // formats and prints encumbrance info to specified window
        void print_encumbrance( ui_adaptor &ui, const catacurses::window &win, int line = -1,
                                const item *selected_clothing = nullptr ) const;
        /** Returns true if the character is wearing power armor */
        bool is_wearing_power_armor( bool *hasHelmet = nullptr ) const;
        /** Returns true if the character is wearing active power */
        bool is_wearing_active_power_armor() const;
        /** Returns true if the player is wearing an active optical cloak */
        bool is_wearing_active_optcloak() const;

        /** Returns strength of any climate control affecting character, for heating and chilling respectively */
        std::pair<int, int> climate_control_strength();

        /** Returns wind resistance provided by armor, etc **/
        std::map<bodypart_id, int> get_wind_resistance( const
                std::map<bodypart_id, std::vector<const item *>> &clothing_map ) const;

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
        bool try_remove_grab( bool attacking = false );
        void try_remove_downed();
        void try_remove_bear_trap();
        void try_remove_lightsnare();
        void try_remove_heavysnare();
        void try_remove_crushed();
        void try_remove_webs();
        void try_remove_impeding_effect();
        // Calculate generic trap escape chance
        bool can_escape_trap( int difficulty, bool manip ) const;

        /** Check against the character's current movement mode */
        bool movement_mode_is( const move_mode_id &mode ) const;
        move_mode_id current_movement_mode() const;

        bool is_running() const;
        bool is_walking() const;
        bool is_crouching() const;
        bool is_runallfours() const;
        bool is_prone() const;

        int footstep_sound() const;
        // the sound clattering items dangling off you can make
        int clatter_sound() const;
        void make_footstep_noise() const;
        void make_clatter_sound() const;

        bool can_switch_to( const move_mode_id &mode ) const;
        steed_type get_steed_type() const;
        virtual void set_movement_mode( const move_mode_id &mode ) = 0;

        /**Determine if character is susceptible to dis_type and if so apply the symptoms*/
        void expose_to_disease( const diseasetype_id &dis_type );
        /**
         * Handles end-of-turn processing.
         */
        void process_turn() override;

        /** Recalculates HP after a change to max strength or enchantment */
        void recalc_hp();

        /** Maintains body wetness and handles the rate at which the player dries */
        void update_body_wetness( const w_point &weather );

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
        bool natural_attack_restricted_on( const bodypart_id &bp ) const;
        bool natural_attack_restricted_on( const sub_bodypart_id &bp ) const;

        int blocks_left;

        double recoil = MAX_RECOIL;

        std::string custom_profession;

        /** Returns true if the player has quiet melee attacks */
        bool is_quiet() const;

        // melee.cpp
        /** Checks for valid block abilities and reduces damage accordingly. Returns true if the player blocks */
        bool block_hit( Creature *source, bodypart_id &bp_hit, damage_instance &dam ) override;
        /** Returns the best item for blocking with */
        item_location best_shield();
        /** Calculates melee weapon wear-and-tear through use, returns true if item is destroyed. */
        bool handle_melee_wear( item_location shield, float wear_multiplier = 1.0f );
        /** Returns a random valid technique */
        matec_id pick_technique( Creature &t, const item_location &weap,
                                 bool crit, bool dodge_counter, bool block_counter, const std::vector<matec_id> &blacklist = {} );
        // Houses the actual picking logic and returns the vector of eligable techniques
        std::vector<matec_id> evaluate_techniques( Creature &t, const item_location &weap,
                bool crit = false, bool dodge_counter = false, bool block_counter = false,
                const std::vector<matec_id> &blacklist = {} );
        void perform_technique( const ma_technique &technique, Creature &t, damage_instance &di,
                                int &move_cost, item_location &cur_weapon );

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
                           bool allow_unarmed = true, int forced_movecost = -1 );
        bool melee_attack_abstract( Creature &t, bool allow_special, const matec_id &force_technique,
                                    bool allow_unarmed = true, int forced_movecost = -1 );

        /** Handles reach melee attacks */
        void reach_attack( const tripoint &p, int forced_movecost = -1 );

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
        /** Returns cost (in stamina) of attacking with given item, or wielded item if nullptr (no modifiers, worst possible is -50) */
        int get_base_melee_stamina_cost( const item *weap = nullptr ) const;
        /** Returns total cost (in stamina) of attacking with given item, or wielded item if nullptr (modified by skill and walk/crouch/prone, worst possible is -50) */
        int get_total_melee_stamina_cost( const item *weap = nullptr ) const;
        /** Gets melee accuracy component from weapon+skills */
        float get_hit_weapon( const item &weap ) const;
        /** Check if we can attack upper limbs **/
        bool can_attack_high() const override;

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
        void roll_all_damage( bool crit, damage_instance &di, bool average, const item &weap,
                              const std::string &attack_vector,
                              const Creature *target, const bodypart_id &bp ) const;
        void roll_damage( const damage_type_id &dt, bool crit, damage_instance &di, bool average,
                          const item &weap, const std::string &attack_vector, float crit_mod ) const;

        /** Returns true if the player should be dead */
        bool is_dead_state() const override;

    private:
        mutable std::optional<bool> cached_dead_state;
    public:
        void set_part_hp_cur( const bodypart_id &id, int set ) override;
        void mod_part_hp_cur( const bodypart_id &id, int set ) override;

        void calc_all_parts_hp( float hp_mod = 0.0, float hp_adjust = 0.0, int str_max = 0,
                                int dex_max = 0, int per_max = 0, int int_max = 0, int healthy_mod = 0,
                                int fat_to_max_hp = 0 );

        /** Returns true if the player has stealthy movement */
        bool is_stealthy() const;
        /** Returns true if the current martial art works with the player's current weapon */
        bool can_melee() const;
        /** Returns value of player's stable footing */
        float stability_roll() const override;
        /** Returns true if the player can learn the entered martial art */
        bool can_autolearn( const matype_id &ma_id ) const;
    private:
        /** Check if an area-of-effect technique has valid targets */
        bool valid_aoe_technique( Creature &t, const ma_technique &technique );
        bool valid_aoe_technique( Creature &t, const ma_technique &technique,
                                  std::vector<Creature *> &targets );
    public:

        /** This handles giving xp for a skill. Returns true on level-up. */
        bool practice( const skill_id &id, int amount, int cap = 99, bool suppress_warning = false );
        /** This handles warning the player that there current activity will not give them xp */
        void handle_skill_warning( const skill_id &id, bool force_warning = false );

        /**
         * Check player capable of wielding an item.
         * @param it Thing to be wielded
         */
        ret_val<void> can_wield( const item &it ) const;

        bool unwield();

        /** Get the formatted name of the currently wielded item (if any) with current gun mode (if gun) */
        std::string weapname() const;
        /** Get the formatted name of the currently wielded item (if any) without current gun mode and ammo */
        std::string weapname_simple() const;
        /** Get the formatted current gun mode (if gun) */
        std::string weapname_mode() const;
        /** Get the formatted current ammo (if gun) */
        std::string weapname_ammo() const;

        // any side effects that might happen when the Character is hit
        /** Handles special defenses from melee attack that hit us (source can be null) */
        void on_hit( Creature *source, bodypart_id bp_hit,
                     float difficulty = INT_MIN, dealt_projectile_attack const *proj = nullptr ) override;
        // any side effects that might happen when the Character hits a Creature
        void did_hit( Creature &target );

        /** Actually hurt the player, hurts a body_part directly, no armor reduction */
        void apply_damage( Creature *source, bodypart_id hurt, int dam,
                           bool bypass_med = false ) override;
        /** Calls Creature::deal_damage and handles damaged effects (waking up, etc.) */
        dealt_damage_instance deal_damage( Creature *source, bodypart_id bp,
                                           const damage_instance &d,
                                           const weakpoint_attack &attack = weakpoint_attack() ) override;
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
        const weakpoint *absorb_hit( const weakpoint_attack &attack, const bodypart_id &bp,
                                     damage_instance &dam ) override;
    protected:
        float generic_weakpoint_skill( skill_id skill_1, skill_id skill_2,
                                       limb_score_id limb_score_1, limb_score_id limb_score_2 ) const;
    public:
        /** The character's skill in hitting a weakpoint */
        float melee_weakpoint_skill( const item &weapon ) const;
        float ranged_weakpoint_skill( const item &weapon ) const;
        float throw_weakpoint_skill() const;
        /**
         * Reduces and mutates du, prints messages about armor taking damage.
         * Requires a roll out of 100
         * @return true if the armor was completely destroyed (and the item must be deleted).
         */
        bool armor_absorb( damage_unit &du, item &armor, const bodypart_id &bp, int roll ) const;
        /**
         * Reduces and mutates du, prints messages about armor taking damage.
         * Requires a roll out of 100
         * @return true if the armor was completely destroyed (and the item must be deleted).
         */
        bool armor_absorb( damage_unit &du, item &armor, const bodypart_id &bp, const sub_bodypart_id &sbp,
                           int roll ) const;
        /**
         * Reduces and mutates du, prints messages about armor taking damage.
         * If the armor is fully destroyed it is replaced
         * @return true if the armor was completely destroyed.
         */
        bool ablative_armor_absorb( damage_unit &du, item &armor, const sub_bodypart_id &bp, int roll );
        // prints a message related to an item if an item is damaged
        void describe_damage( damage_unit &du, item &armor ) const;
        /**
         * Check for passive bionics that provide armor, and returns the armor bonus
         * This is called from player::passive_absorb_hit
         */
        float bionic_armor_bonus( const bodypart_id &bp, const damage_type_id &dt ) const;
        /** Returns the armor bonus against given type from martial arts buffs */
        int mabuff_armor_bonus( const damage_type_id &type ) const;
        // --------------- Mutation Stuff ---------------
        // In newcharacter.cpp
        /** Returns the id of a random starting trait that costs >= 0 points */
        trait_id random_good_trait();
        /** Returns the id of a random starting trait that costs < 0 points */
        trait_id random_bad_trait();
        /** Returns the id of a random trait matching the given predicate */
        trait_id get_random_trait( const std::function<bool( const mutation_branch & )> &func );
        void randomize_cosmetic_trait( const std::string &mutation_type );

        // In mutation.cpp
        /** Returns true if the player has a conflicting trait to the entered trait
         *  Uses has_opposite_trait(), has_lower_trait(), and has_higher_trait() to determine conflicts.
         */
        bool has_conflicting_trait( const trait_id &flag ) const;
        /** Returns all player's traits conflicting with the entered trait */
        std::unordered_set<trait_id> get_conflicting_traits( const trait_id &flag ) const;
        /** Returns true if the player has a trait which upgrades into the entered trait */
        bool has_lower_trait( const trait_id &flag ) const;
        /** Returns player's traits which upgrade into the entered trait */
        std::unordered_set<trait_id> get_lower_traits( const trait_id &flag ) const;
        /** Returns true if the player has a trait which is a replacement of the entered trait */
        bool has_replacement_trait( const trait_id &flag ) const;
        /** Returns player's traits which replace the entered trait */
        std::unordered_set<trait_id> get_replacement_traits( const trait_id &flag ) const;
        /** Returns true if the player has a trait which adds to the entered trait */
        bool has_addition_trait( const trait_id &flag ) const;
        /** Returns player's traits which add to the entered trait */
        std::unordered_set<trait_id> get_addition_traits( const trait_id &flag ) const;
        /** Returns true if the player has a trait that shares a type with the entered trait */
        bool has_same_type_trait( const trait_id &flag ) const;
        /** Returns player's traits that share a type with the entered trait */
        std::unordered_set<trait_id> get_same_type_traits( const trait_id &flag ) const;
        /** Returns true if the entered trait may be purified away
         *  Defaults to true
         */
        bool purifiable( const trait_id &flag ) const;
        /** Returns a dream's description selected randomly from the player's highest mutation category */
        std::string get_category_dream( const mutation_category_id &cat, int strength ) const;
        /** Returns true if the player has the entered trait */
        bool has_trait( const trait_id &b ) const override;
        /** Returns true if the player has the entered trait with the desired variant */
        bool has_trait_variant( const trait_and_var & ) const;
        /** Returns true if the player has the entered starting trait */
        bool has_base_trait( const trait_id &b ) const;
        /** Returns true if player has a trait with a flag */
        bool has_trait_flag( const json_character_flag &b ) const;
        /** Returns true if player has a bionic with a flag */
        bool has_bionic_with_flag( const json_character_flag &flag ) const;
        /** Returns true if the player has any bodypart with a flag */
        bool has_bodypart_with_flag( const json_character_flag &flag ) const;
        /** Returns count of traits with a flag */
        int count_trait_flag( const json_character_flag &b ) const;
        /** Returns count of bionics with a flag */
        int count_bionic_with_flag( const json_character_flag &flag ) const;
        /** Returns count of bodyparts with a flag */
        int count_bodypart_with_flag( const json_character_flag &flag ) const;
        /** This is to prevent clang complaining about overloading a virtual function, the creature version uses monster flags so confusion is unlikely. */
        using Creature::has_flag;
        /** Returns true if player has a trait, bionic, effect, bodypart, or martial arts buff with a flag */
        bool has_flag( const json_character_flag &flag ) const;
        /** Returns the count of traits, bionics, effects, bodyparts, and martial arts buffs with a flag */
        int count_flag( const json_character_flag &flag ) const;
        /** Returns the trait id with the given invlet, or an empty string if no trait has that invlet */
        trait_id trait_by_invlet( int ch ) const;

        /** Toggles a trait on the player and in their mutation list */
        void toggle_trait( const trait_id &, const std::string & = "" );
        /** Add or removes a mutation on the player, but does not trigger mutation loss/gain effects. */
        void set_mutations( const std::vector<trait_id> &traits );
        void set_mutation( const trait_id &, const mutation_variant * = nullptr );
        /** Switches the variant of the given mutation, if the player has that mutation */
        void set_mut_variant( const trait_id &, const mutation_variant * );
    protected:
        // Set a mutation, but don't do any of the necessary updates
        // Only call this from one of the above two functions
        void set_mutation_unsafe( const trait_id &, const mutation_variant * = nullptr );
    public:
        // Do the mutation updates necessary when adding a mutation (nonspecific cache updates)
        void do_mutation_updates();
        void unset_mutation( const trait_id & );
        /**Unset switched mutation and set target mutation instead, if safe mutates towards the target mutation*/
        void switch_mutations( const trait_id &switched, const trait_id &target, bool start_powered,
                               bool safe = false );

        bool can_power_mutation( const trait_id &mut ) const;
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
        bool cant_do_mounted( bool msg = true ) const;
        bool is_mounted() const;
        bool check_mount_will_move( const tripoint &dest_loc );
        bool check_mount_is_spooked();
        void dismount();
        void forced_dismount();

        bool is_deaf() const;
        bool is_mute() const;
        // Get the specified limb score. If bp is defined, only the scores from that body part type are summed.
        // override forces the limb score to be affected by encumbrance/wounds (-1 == no override).
        float get_limb_score( const limb_score_id &score,
                              const body_part_type::type &bp = body_part_type::type::num_types,
                              int override_encumb = -1, int override_wounds = -1 ) const;
        float manipulator_score( const std::map<bodypart_str_id, bodypart> &body,
                                 body_part_type::type type, int override_encumb, int override_wounds ) const;

        bool has_min_manipulators() const;
        // technically this is "has more than one arm"
        bool has_two_arms_lifting() const;
        // Return all the limb special attacks the character has, if the parent limb isn't too encumbered
        std::set<matec_id> get_limb_techs() const;
        int get_working_arm_count() const;
        /** Returns true if enough of your legs are working
          * Currently requires all, new morphologies could be different
          */
        bool enough_working_legs() const;
        /** Returns the number of functioning legs */
        int get_working_leg_count() const;
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
        /**
         * @return Skills of which this NPC has a higher level than the given player. In other
         * words: skills this NPC could teach the player.
         */
        std::vector<skill_id> skills_offered_to( const Character *you ) const;
        /**
         * Proficiencies we know that the character doesn't
         */
        std::vector<proficiency_id> proficiencies_offered_to( const Character *guy ) const;
        /**
         * Martial art styles that we known, but the player p doesn't.
         */
        std::vector<matype_id> styles_offered_to( const Character *you ) const;
        /**
         * Spells that the NPC knows but that the player p doesn't.
         * not const because get_spell isn't const and both this and p call it
         */
        std::vector<spell_id> spells_offered_to( const Character *you ) const;

        int calc_spell_training_cost( bool knows, int difficulty, int level ) const;

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

        static const std::vector<material_id> fleshy;
        bool made_of( const material_id &m ) const override;
        bool made_of_any( const std::set<material_id> &ms ) const override;

    private:
        /** Retrieves a stat mod of a mutation. */
        int get_mod( const trait_id &mut, const std::string &arg ) const;
        /** Applies skill-based boosts to stats **/
        void apply_skill_boost();
    protected:

        void on_move( const tripoint_abs_ms &old_pos ) override;
        void do_skill_rust();
        /** Applies stat mods to character. */
        void apply_mods( const trait_id &mut, bool add_remove );

        /** Applies encumbrance from mutations and bionics only */
        void mut_cbm_encumb( std::map<bodypart_id, encumbrance_data> &vals ) const;

        void apply_mut_encumbrance( std::map<bodypart_id, encumbrance_data> &vals ) const;

        /** Applies encumbrance from BMI */
        void calc_bmi_encumb( std::map<bodypart_id, encumbrance_data> &vals ) const;
        void calc_bmi_encumb() const;

    public:
        /** Recalculate encumbrance for all body parts. */
        void calc_encumbrance();
        /** Calculate any discomfort your current clothes are causing. */
        void calc_discomfort();
        /** Recalculate encumbrance for all body parts as if `new_item` was also worn. */
        void calc_encumbrance( const item &new_item );
        // recalculates bodyparts based on enchantments modifying them and the default anatomy.
        void recalculate_bodyparts();
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
        /** Returns the arpen bonus from martial arts buffs*/
        int mabuff_arpen_bonus( const damage_type_id &type ) const;
        /** Returns the damage multiplier to given type from martial arts buffs */
        float mabuff_damage_mult( const damage_type_id &type ) const;
        /** Returns the flat damage bonus to given type from martial arts buffs, applied after the multiplier */
        int mabuff_damage_bonus( const damage_type_id &type ) const;
        /** Returns the flat penalty to move cost of attacks. If negative, that's a bonus. Applied after multiplier. */
        int mabuff_attack_cost_penalty() const;
        /** Returns the multiplier on move cost of attacks. */
        float mabuff_attack_cost_mult() const;
        /** Returns true if player has a MA buff with a flag */
        bool has_mabuff_flag( const json_character_flag &flag ) const;
        /* Returns the number of MA buffs with the flag*/
        int count_mabuff_flag( const json_character_flag &flag ) const;

        /** Handles things like destruction of armor, etc. */
        void mutation_effect( const trait_id &mut, bool worn_destroyed_override );
        /** Handles what happens when you lose a mutation. */
        void mutation_loss_effect( const trait_id &mut );

        bool has_active_mutation( const trait_id &b ) const;

        time_duration get_cost_timer( const trait_id &mut_id ) const;
        void set_cost_timer( const trait_id &mut, time_duration set );
        void mod_cost_timer( const trait_id &mut, time_duration mod );

        /** Picks a random valid mutation and gives it to the Character, possibly removing/changing others along the way */
        void mutate( const int &true_random_chance, bool use_vitamins );
        void mutate( );
        /** Returns true if the player doesn't have the mutation or a conflicting one and it complies with the allowed typing */
        bool mutation_ok( const trait_id &mutation, bool allow_good, bool allow_bad, bool allow_neutral,
                          const vitamin_id &mut_vit ) const;
        bool mutation_ok( const trait_id &mutation, bool allow_good, bool allow_bad,
                          bool allow_neutral ) const;
        /** Roll, based on instability, whether next mutation should be good or bad */
        bool roll_bad_mutation() const;
        /** Opens a menu which allows players to choose from a list of mutations */
        bool mutation_selector( const std::vector<trait_id> &prospective_traits,
                                const mutation_category_id &cat, const bool &use_vitamins );
        /** Picks a random valid mutation in a category and mutate_towards() it */
        void mutate_category( const mutation_category_id &mut_cat, bool use_vitamins,
                              bool true_random = false );
        void mutate_category( const mutation_category_id &mut_cat );
        /** Mutates toward one of the given mutations, upgrading or removing conflicts if necessary */
        bool mutate_towards( std::vector<trait_id> muts, const mutation_category_id &mut_cat,
                             int num_tries = INT_MAX, bool use_vitamins = true, bool removed_base_trait = false );
        /** Mutates toward the entered mutation, upgrading or removing conflicts if necessary */
        bool mutate_towards( const trait_id &mut, const mutation_category_id &mut_cat,
                             const mutation_variant *chosen_var = nullptr, bool use_vitamins = true,
                             bool removed_base_trait = false );
        bool mutate_towards( const trait_id &mut, const mutation_variant *chosen_var = nullptr );
        /** Removes a mutation, downgrading to the previous level if possible */
        void remove_mutation( const trait_id &mut, bool silent = false );
        /** Returns true if the player has the entered mutation child flag */
        bool has_child_flag( const trait_id &flag ) const;
        /** Removes the mutation's child flag from the player's list */
        void remove_child_flag( const trait_id &flag );
        /** Try to cross The Threshold */
        void test_crossing_threshold( const mutation_category_id &mutation_category );
        /** Returns how many steps are required to reach a mutation */
        int mutation_height( const trait_id &mut );
        /** Recalculates mutation_category_level[] values for the player */
        void calc_mutation_levels();
        /** Returns a weighted list of mutation categories based on blood vitamin levels */
        weighted_int_list<mutation_category_id> get_vitamin_weighted_categories() const;
        /** Recalculates mutation drench protection for all bodyparts (ignored/good/neutral stats) */
        void drench_mut_calc();

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
        float mutation_armor( bodypart_id bp, const damage_type_id &dt ) const;
        float mutation_armor( bodypart_id bp, const damage_unit &du ) const;

        /** gives every mutation in a category, optionally including post-thresh mutations */
        void give_all_mutations( const mutation_category_trait &category, bool include_postthresh = false );
        /** unsets all mutations */
        void unset_all_mutations();

        // --------------- Bionic Stuff ---------------
        /** Handles bionic activation effects of the entered bionic, returns if anything activated */
        bool activate_bionic( bionic &bio, bool eff_only = false, bool *close_bionics_ui = nullptr );
        std::vector<bionic_id> get_bionics() const;
        std::vector<const item *> get_pseudo_items() const;
        void invalidate_pseudo_items();
        /** Finds the highest UID for installed bionics and caches the next valid UID **/
        void update_last_bionic_uid() const;
        /** Returns the next valid UID for a bionic installation **/
        bionic_uid generate_bionic_uid() const;
        /** Returns true if the player has the entered bionic id */
        bool has_bionic( const bionic_id &b ) const;
        /** Returns true if the player has the entered bionic id and it is powered on */
        bool has_active_bionic( const bionic_id &b ) const;
        /**Returns true if the player has any bionic*/
        bool has_any_bionic() const;
        /**Return bionic_id of bionics able to use it as fuel*/
        std::vector<bionic_id> get_bionic_fueled_with_muscle() const;
        /**Return bionic_id of fueled bionics*/
        std::vector<bionic_id> get_fueled_bionics() const;
        /**Returns bionic_id of first remote fueled bionic found*/
        bionic_id get_remote_fueled_bionic() const;

        /**Return list of available fuel sources that are not empty for this bionic*/
        std::vector<item *> get_bionic_fuels( const bionic_id &bio );
        /** Returns not-empty UPS connected to cable charger bionic */
        std::vector<item *> get_cable_ups();
        /** Returns solar items connected to cable charger bionic */
        std::vector<item *> get_cable_solar();
        /** Returns vehicles connected to cable charger bionic */
        std::vector<vehicle *> get_cable_vehicle() const;

        /**Get stat bonus from bionic*/
        int get_mod_stat_from_bionic( const character_stat &Stat ) const;
        // route for overmap-scale traveling
        std::vector<tripoint_abs_omt> omt_path;
        bool is_using_bionic_weapon() const;
        bionic_uid get_weapon_bionic_uid() const;

        /** Handles bionic effects over time of the entered bionic */
        void process_bionic( bionic &bio );
        /** Checks if bionic can be deactivated (e.g. it's not incapacitated and power level is sufficient)
         *  returns either success or failure with log message */
        ret_val<void> can_deactivate_bionic( bionic &bio, bool eff_only = false ) const;
        /** Handles bionic deactivation effects of the entered bionic, returns if anything
         *  deactivated */
        bool deactivate_bionic( bionic &bio, bool eff_only = false );
        /* Forces bionic deactivation */
        void force_bionic_deactivation( bionic &bio );
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
        bool has_enough_anesth( const itype &cbm, Character &patient ) const;
        bool has_enough_anesth( const itype &cbm ) const;
        void consume_anesth_requirement( const itype &cbm, Character &patient );
        /**Has the required equipment for manual installation*/
        bool has_installation_requirement( const bionic_id &bid ) const;
        void consume_installation_requirement( const bionic_id &bid );
        /** Handles process of introducing patient into anesthesia during Autodoc operations. Requires anesthesia kits or NOPAIN mutation */
        void introduce_into_anesthesia( const time_duration &duration, Character &installer,
                                        bool needs_anesthesia );
        /** Finds the first bionic instance that matches the bionic_id */
        std::optional<bionic *> find_bionic_by_type( const bionic_id &b ) const;
        /** Finds the bionic with specified UID */
        std::optional<bionic *> find_bionic_by_uid( bionic_uid bio_uid ) const;
        /** Removes a bionic from my_bionics[] */
        void remove_bionic( const bionic &bio );
        /** Adds a bionic to my_bionics[] */
        bionic_uid add_bionic( const bionic_id &b, bionic_uid parent_uid = 0, bool suppress_debug = false );
        /**Calculate skill bonus from tiles in radius*/
        float env_surgery_bonus( int radius ) const;
        /** Calculate skill for (un)installing bionics */
        float bionics_adjusted_skill( bool autodoc, int skill_level = -1 ) const;
        /** Calculate non adjusted skill for (un)installing bionics */
        int bionics_pl_skill( bool autodoc, int skill_level = -1 ) const;
        /**Is the installation possible*/
        bool can_install_bionics( const itype &type, Character &installer, bool autodoc = false,
                                  int skill_level = -1 ) const;
        /** Is this bionic elligible to be installed in the player? */
        ret_val<void> is_installable( const item *it, bool by_autodoc ) const;
        std::map<bodypart_id, int> bionic_installation_issues( const bionic_id &bioid ) const;
        /** Initialize all the values needed to start the operation player_activity */
        bool install_bionics( const itype &type, Character &installer, bool autodoc = false,
                              int skill_level = -1 );
        /**Success or failure of installation happens here*/
        void perform_install( const bionic_id &bid, bionic_uid upbio_uid, int difficulty, int success,
                              int pl_skill, const std::string &installer_name,
                              const std::vector<trait_id> &trait_to_rem, const tripoint &patient_pos );
        void bionics_install_failure( const bionic_id &bid, const std::string &installer, int difficulty,
                                      int success, float adjusted_skill, const tripoint &patient_pos );
        /**
         * Try to wield a contained item consuming moves proportional to weapon skill and volume.
         * @param container Container containing the item to be wielded
         * @param internal_item reference to contained item to wield.
         * @param penalties Whether item volume and temporary effects (e.g. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         */
        bool wield_contents( item &container, item *internal_item = nullptr, bool penalties = true,
                             int base_cost = INVENTORY_HANDLING_PENALTY );
        /** Uses a tool */
        void use( int inventory_position );
        /** Uses a tool at location */
        void use( item_location loc, int pre_obtain_moves = -1, std::string const &method = {} );
        /** Uses the current wielded weapon */
        void use_wielded();
        /** Wear item; returns false on fail. If interactive is false, don't alert the player or drain moves on completion. */
        std::optional<std::list<item>::iterator>
        wear( int pos, bool interactive = true );

        /** Wear item; returns false on fail. If interactive is false, don't alert the player or drain moves on completion.
        * @param item_wear item_location of item to be worn.
        * @param interactive Alert player and drain moves if true.
        */
        std::optional<std::list<item>::iterator>
        wear( item_location item_wear, bool interactive = true );

        /** Used for eating object at a location. Removes item if all of it was consumed.
        *   @returns trinary enum NONE, SOME or ALL amount consumed.
        */
        trinary consume( item_location loc, bool force = false );

        /** Used for eating a particular item that doesn't need to be in inventory.
         *  @returns trinary enum NONE, SOME or ALL (doesn't remove).
         */
        trinary consume( item &target, bool force = false );

        /**
         * Stores an item inside another consuming moves proportional to weapon skill and volume.
         * Note: This method bypasses pocket settings.
         * @param container Container in which to store the item
         * @param put Item to add to the container
         * @param penalties Whether item volume and temporary effects (e.g. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         */
        void store( item &container, item &put, bool penalties = true,
                    int base_cost = INVENTORY_HANDLING_PENALTY,
                    item_pocket::pocket_type pk_type = item_pocket::pocket_type::CONTAINER,
                    bool check_best_pkt = false );
        void store( item_pocket *pocket, item &put, bool penalties = true,
                    int base_cost = INVENTORY_HANDLING_PENALTY );
        /**Is The uninstallation possible*/
        bool can_uninstall_bionic( const bionic &bio, Character &installer, bool autodoc = false,
                                   int skill_level = -1 ) const;
        /** Initialize all the values needed to start the operation player_activity */
        bool uninstall_bionic( const bionic &bio, Character &installer, bool autodoc = false,
                               int skill_level = -1 );
        /**Success or failure of removal happens here*/
        void perform_uninstall( const bionic &bio, int difficulty, int success, int pl_skill );
        /**When a player fails the surgery*/
        void bionics_uninstall_failure( int difficulty, int success, float adjusted_skill );

        /**When a critical failure occurs*/
        void roll_critical_bionics_failure( const bodypart_id &bp );

        /**Used by monster to perform surgery*/
        bool uninstall_bionic( const bionic &bio, monster &installer, Character &patient,
                               float adjusted_skill );
        /**When a monster fails the surgery*/
        void bionics_uninstall_failure( monster &installer, Character &patient, int difficulty, int success,
                                        float adjusted_skill );
        void on_worn_item_transform( const item &old_it, const item &new_it );

        /**
         * Starts activity to remove gunmod after unloading any contained ammo.
         * Returns true on success (activity has been started)
         */
        bool gunmod_remove( item &gun, item &mod );

        /** Starts activity to install gunmod having warned user about any risk of failure or irremovable mods s*/
        void gunmod_add( item &gun, item &mod );

        /** Starts activity to install toolmod */
        void toolmod_add( item_location tool, item_location mod );

        /**
         * Attempt to mend an item (fix any current faults)
         * @param obj Object to mend
         * @param interactive if true prompts player when multiple faults, otherwise mends the first
         */
        void mend_item( item_location &&obj, bool interactive = true );

        bool list_ammo( const item_location &base, std::vector<item::reload_option> &ammo_list,
                        bool empty = true ) const;
        /**
         * Select suitable ammo with which to reload the item
         * @param base Item to select ammo for
         * @param prompt force display of the menu even if only one choice
         * @param empty allow selection of empty magazines
         */
        item::reload_option select_ammo( const item_location &base, bool prompt = false,
                                         bool empty = true ) const;

        /** Select ammo from the provided options */
        item::reload_option select_ammo( const item_location &base, std::vector<item::reload_option> opts,
                                         const std::string &name_override = std::string() ) const;

        void process_items();
        void leak_items();
        /** Search surrounding squares for traps (and maybe other things in the future). */
        void search_surroundings();
        /**Handle heat from exothermic power generation*/
        void heat_emission( const bionic &bio, units::energy fuel_energy );
        /**Applies modifier to fuel_efficiency and returns the resulting efficiency*/
        float get_effective_efficiency( const bionic &bio, float fuel_efficiency ) const;

        units::energy get_power_level() const;
        units::energy get_max_power_level() const;
        void mod_power_level( const units::energy &npower );
        void mod_max_power_level_modifier( const units::energy &npower_max );
        void set_power_level( const units::energy &npower );
        void set_max_power_level( const units::energy &capacity );
        void set_max_power_level_modifier( const units::energy &capacity );
        void update_bionic_power_capacity();
        bool is_max_power() const;
        bool has_power() const;
        bool has_max_power() const;
        bool enough_power_for( const bionic_id &bid ) const;
        /** Handles and displays detailed character info for the '@' screen */
        void disp_info( bool customize_character = false );
        void conduct_blood_analysis();
        // --------------- Generic Item Stuff ---------------

        struct has_mission_item_filter {
            int mission_id;
            bool operator()( const item &it ) const {
                return it.mission_id == mission_id || it.has_any_with( [&]( const item & it ) {
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
            return worn.is_worn( thing );
        }

        bool is_worn_module( const item &thing ) const {
            return worn.is_worn_module( thing );
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
         * @param charges_in_it the amount of charges to be handled (default is whole)
         * @param bulk_cost Calculate costs excluding overhead, such as when loading and unloading items in bulk (default is false)
         * @return cost in moves ranging from 0 to MAX_HANDLING_COST
         */
        int item_handling_cost( const item &it, bool penalties = true,
                                int base_cost = INVENTORY_HANDLING_PENALTY, int charges_in_it = -1, bool bulk_cost = false ) const;

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
        std::optional<std::list<item>::iterator>
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

        void clear_worn();

        // returns a list of all item_location the character has, including items contained in other items.
        // only for CONTAINER pocket type; does not look for magazines
        std::vector<item_location> all_items_loc();
        // Returns list of all the top level item_location the character has. Includes worn items but excludes items held on hand.
        std::vector<item_location> top_items_loc();
        /** Return the item pointer of the item with given invlet, return nullptr if
         * the player does not have such an item with that invlet. Don't use this on npcs.
         * Only use the invlet in the user interface, otherwise always use the item position. */
        item *invlet_to_item( int invlet ) const;

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
        item_location used_weapon() const;
        item_location used_weapon();
        /*@}*/

        /**
         * Adds the item to the character's worn items or wields it, or prompts if the Character cannot pick it up.
         * @original_inventory_item set if the item was already in the characters inventory (wielded, worn, in different pocket) and is being moved.
         * @avoid is the item to not put @it into
         */
        item_location i_add( item it, bool should_stack = true, const item *avoid = nullptr,
                             const item *original_inventory_item = nullptr, bool allow_drop = true,
                             bool allow_wield = true, bool ignore_pkt_settings = false );
        item_location i_add( item it, int &copies_remaining, bool should_stack = true,
                             const item *avoid = nullptr,
                             const item *original_inventory_item = nullptr, bool allow_drop = true,
                             bool allow_wield = true, bool ignore_pkt_settings = false );
        /** tries to add to the character's inventory without a popup. returns nullptr if it fails. */
        item_location try_add( item it, const item *avoid = nullptr,
                               const item *original_inventory_item = nullptr, bool allow_wield = true,
                               bool ignore_pkt_settings = false );
        item_location try_add( item it, int &copies_remaining, const item *avoid = nullptr,
                               const item *original_inventory_item = nullptr, bool allow_wield = true,
                               bool ignore_pkt_settings = false );

        ret_val<item_location> i_add_or_fill( item &it, bool should_stack = true,
                                              const item *avoid = nullptr,
                                              const item *original_inventory_item = nullptr, bool allow_drop = true,
                                              bool allow_wield = true, bool ignore_pkt_settings = false );

        /**
         * Try to pour the given liquid into the given container/vehicle. The transferred charges are
         * removed from the liquid item. Check the charges of afterwards to see if anything has
         * been transferred at all.
         * The functions do not consume any move points.
         * @return Whether anything has been moved at all. `false` indicates the transfer is not
         * possible at all. `true` indicates at least some of the liquid has been moved.
         */
        /**@{*/
        bool pour_into( item_location &container, item &liquid, bool ignore_settings );
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
         *  An optional qty can be provided (and will perform better than separate calls).
         *  @original_inventory_item set if the item was already in the characters inventory (wielded, worn, in different pocket) and is being moved.
         */
        bool i_add_or_drop( item &it, int qty = 1, const item *avoid = nullptr,
                            const item *original_inventory_item = nullptr );

        /** Drops items at player location
        *  An optional qty can be provided (and will perform better than separate calls).
        */
        bool i_drop_at( item &it, int qty = 1 );

        /**
         * Check any already unsealed pockets in items pointed to by `containers`
         * and propagate the unsealed status through the container tree. In the
         * process the player may be asked to handle containers or spill contents,
         * so make sure all unsealed containers are passed to this function in a
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
        int ammo_count_for( const item_location &gun ) const;

        /**
         * Whether a tool or gun is potentially reloadable (optionally considering a specific ammo)
         * @param it Thing to be reloaded
         * @param ammo if set also check item currently compatible with this specific ammo or magazine
         * @note items currently loaded with a detachable magazine are considered reloadable
         * @note items with integral magazines are reloadable if free capacity permits (+/- ammo matches)
         */
        bool can_reload( const item &it, const item *ammo = nullptr ) const;

        /** Same as `Character::can_reload`, but checks for attached gunmods as well. */
        hint_rating rate_action_reload( const item &it ) const;
        /** Whether a tool or a gun can be unloaded. */
        hint_rating rate_action_unload( const item &it ) const;
        hint_rating rate_action_insert( const item_location &loc ) const;
        /**
          * So far only called by unload() from game.cpp
          * @avoid - do not put @it into @avoid
         * @original_inventory_item set if the item was already in the characters inventory (wielded, worn, in different pocket) and is being moved.
          */
        bool add_or_drop_with_msg( item &it, bool unloading = false, const item *avoid = nullptr,
                                   const item *original_inventory_item = nullptr );
        /**
         * Unload item.
         * @param bypass_activity If item requires an activity for its unloading, unload item immediately instead.
         */
        bool unload( item_location &loc, bool bypass_activity = false,
                     const item_location &new_container = item_location::nowhere );
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

        /** True if unarmed */
        bool unarmed_attack() const;

        /// Checks for items, tools, and vehicles with the Lifting quality near the character
        /// returning the largest weight liftable by an item in range.
        units::mass best_nearby_lifting_assist() const;
        /// Alternate version if you need to specify a different origin point for nearby vehicle sources of lifting
        /// used for operations on distant objects (e.g. vehicle installation/uninstallation)
        units::mass best_nearby_lifting_assist( const tripoint &world_pos ) const;

        // weapon + worn (for death, etc)
        std::vector<item *> inv_dump();
        std::vector<const item *> inv_dump() const;
        units::mass weight_carried() const;
        units::volume volume_carried() const;

        units::length max_single_item_length() const;
        units::volume max_single_item_volume() const;

        /// Sometimes we need to calculate hypothetical volume or weight.  This
        /// struct offers two possible tweaks: a collection of items and
        /// counts to remove, or an entire replacement inventory.
        struct item_tweaks {
            item_tweaks() : without_items( std::nullopt ), replace_inv( std::nullopt ) {}
            explicit item_tweaks( const std::map<const item *, int> &w ) :
                without_items( std::cref( w ) )
            {}
            explicit item_tweaks( const inventory &r ) :
                replace_inv( std::cref( r ) )
            {}
            const std::optional<std::reference_wrapper<const std::map<const item *, int>>> without_items;
            const std::optional<std::reference_wrapper<const inventory>> replace_inv;
        };

        units::mass weight_carried_with_tweaks( const item_tweaks &tweaks ) const;
        units::mass weight_carried_with_tweaks( const std::vector<std::pair<item_location, int>>
                                                &locations ) const;
        units::volume volume_carried_with_tweaks( const item_tweaks &tweaks ) const;
        units::volume volume_carried_with_tweaks( const std::vector<std::pair<item_location, int>>
                &locations ) const;
        units::mass weight_capacity() const override;
        units::volume volume_capacity() const;
        units::volume volume_capacity_with_tweaks( const item_tweaks &tweaks ) const;
        units::volume volume_capacity_with_tweaks( const std::vector<std::pair<item_location, int>>
                &locations ) const;
        units::volume free_space() const;
        /**
         * Returns the total volume of all worn holsters.
        */
        units::volume holster_volume() const;

        /**
         * Used and total holsters
        */
        int used_holsters() const;
        int total_holsters() const;
        units::volume free_holster_volume() const;

        // this is just used for pack rat maybe should be moved to the above more robust functions
        int empty_holsters() const;

        /**
         * Returns the total volume of all pockets less than or equal to the volume passed in
         * @param volume threshold for pockets to be considered
        */
        units::volume small_pocket_volume( const units::volume &threshold = 1000_ml ) const;

        /** Note that we've read a book at least once. **/
        virtual bool has_identified( const itype_id &item_id ) const = 0;
        book_mastery get_book_mastery( const item &book ) const;
        virtual void identify( const item &item ) = 0;
        /** Calculates the total fun bonus relative to this character's traits and chapter progress */
        bool fun_to_read( const item &book ) const;
        int book_fun_for( const item &book, const Character &p ) const;

        bool can_pickVolume( const item &it, bool safe = false, const item *avoid = nullptr,
                             bool ignore_pkt_settings = true ) const;
        bool can_pickVolume_partial( const item &it, bool safe = false, const item *avoid = nullptr,
                                     bool ignore_pkt_settings = true ) const;
        bool can_pickWeight( const item &it, bool safe = true ) const;
        bool can_pickWeight_partial( const item &it, bool safe = true ) const;

        /**
          * What is the best pocket to put @it into?
          * @param it the item to try and find a pocket for.
          * @param avoid pockets in this item are not taken into account.
          * @param ignore_settings whether to ignore pocket restriction settings
          *
          * @returns nullptr in the value of the returned pair if no valid pocket was found.
          */
        std::pair<item_location, item_pocket *> best_pocket( const item &it, const item *avoid = nullptr,
                bool ignore_settings = false );

        /**
         * Collect all pocket data (with parent and nest levels added) that the character has.
         * @param filter only collect pockets that match the filter.
         * @param sort_func customizable sorting function.
         *
         * @returns pocket_data_with_parent vector to return.
         */
        std::vector<pocket_data_with_parent> get_all_pocket_with_parent(
            const std::function<bool( const item_pocket * )> &filter = return_true<const item_pocket *>,
            const std::function<bool( const pocket_data_with_parent &a, const pocket_data_with_parent &b )>
            *sort_func = nullptr );

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
        ret_val<void> can_wear( const item &it, bool with_equip_change = false ) const;
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
        ret_val<void> can_unwield( const item &it ) const;
        /**
         * Check player capable of dropping an item.
         * @param it Thing to be unwielded
         */
        ret_val<void> can_drop( const item &it ) const;

        void drop_invalid_inventory();
        // this cache is for checking if items in the character's inventory can't actually fit into other items they are inside of
        void invalidate_inventory_validity_cache();

        void invalidate_weight_carried_cache();
        /** Returns all items that must be taken off before taking off this item */
        std::list<item *> get_dependent_worn_items( const item &it );
        /** Drops an item to the specified location */
        void drop( item_location loc, const tripoint &where );
        virtual void drop( const drop_locations &what, const tripoint &target, bool stash = false );
        /** Assigns character activity to pick up items from the given drop_locations.
         *  Requires sufficient storage; items cannot be wielded or worn from this activity.
         */
        void pick_up( const drop_locations &what );

        bool is_wielding( const item &target ) const;

        bool covered_with_flag( const flag_id &flag, const body_part_set &parts ) const;
        bool is_waterproof( const body_part_set &parts ) const;

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
        /** Returns pointer of the first worn item with a given id. */
        item *item_worn_with_id( const itype_id &id );

        // drawing related stuff
        /**
         * Returns a list of the IDs of overlays on this character,
         * sorted from "lowest" to "highest".
         *
         * Only required for rendering.
         */
        std::vector<std::pair<std::string, std::string>> get_overlay_ids() const;

        // --------------- Skill Stuff ---------------
        float get_skill_level( const skill_id &ident ) const;
        float get_skill_level( const skill_id &ident, const item &context ) const;
        int get_knowledge_level( const skill_id &ident ) const;
        float get_knowledge_plus_progress( const skill_id &ident ) const;
        int get_knowledge_level( const skill_id &ident, const item &context ) const;
        float get_average_skill_level( const skill_id &ident ) const;

        float get_greater_skill_or_knowledge_level( const skill_id &ident ) const;


        SkillLevelMap get_all_skills() const;
        SkillLevel &get_skill_level_object( const skill_id &ident );
        const SkillLevel &get_skill_level_object( const skill_id &ident ) const;

        void set_skill_level( const skill_id &ident, int level );
        void mod_skill_level( const skill_id &ident, int delta );
        void set_knowledge_level( const skill_id &ident, int level );
        void mod_knowledge_level( const skill_id &ident, int delta );
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

        // Mental skills and stats
        /** Returns the player's reading speed as a percentage*/
        int read_speed() const;
        /** Returns a value used when attempting to convince NPC's of something false*/
        int lie_skill() const;
        /** Returns a value used when attempting to convince NPC's of something true*/
        int persuade_skill() const;
        /** Returns a value used when attempting to intimidate NPC's */
        int intimidation() const;

        void set_skills_from_hobbies();

        // --------------- Proficiency Stuff ----------------
        bool has_proficiency( const proficiency_id &prof ) const;
        float get_proficiency_practice( const proficiency_id &prof ) const;
        time_duration get_proficiency_practiced_time( const proficiency_id &prof ) const;
        bool has_prof_prereqs( const proficiency_id &prof ) const;
        void add_proficiency( const proficiency_id &prof, bool ignore_requirements = false );
        void lose_proficiency( const proficiency_id &prof, bool ignore_requirements = false );
        bool practice_proficiency( const proficiency_id &prof, const time_duration &amount,
                                   const std::optional<time_duration> &max = std::nullopt );
        time_duration proficiency_training_needed( const proficiency_id &prof ) const;
        void set_proficiency_practiced_time( const proficiency_id &prof, int turns );
        std::vector<display_proficiency> display_proficiencies() const;
        std::vector<proficiency_id> known_proficiencies() const;
        std::vector<proficiency_id> learning_proficiencies() const;
        int get_proficiency_bonus( const std::string &category, proficiency_bonus_type prof_bonus ) const;
        void add_default_background();
        void set_proficiencies_from_hobbies();

        // tests only!
        void set_proficiency_practice( const proficiency_id &id, const time_duration &amount );

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
        bool cast_spell( spell &sp, bool fake_spell, const std::optional<tripoint> &target );

        /** Called when a player triggers a trap, returns true if they don't set it off */
        bool avoid_trap( const tripoint &pos, const trap &tr ) const override;

        //returns true if the warning is now beyond final and results in hostility.
        bool add_faction_warning( const faction_id &id ) const;
        int current_warnings_fac( const faction_id &id );
        bool beyond_final_warning( const faction_id &id );

        /**
         * Helper function for player::read.
         *
         * @param book Book to read
         * @param reasons Starting with g->u, for each player/NPC who cannot read, a message will be pushed back here.
         * @returns nullptr, if neither the player nor his followers can read to the player, otherwise the player/NPC
         * who can read and can read the fastest
         */
        const Character *get_book_reader( const item &book, std::vector<std::string> &reasons ) const;

        /**
         * Helper function for get_book_reader
         * @warning This function assumes that the everyone is able to read
         *
         * @param book The book being read
         * @param reader the player/NPC who's reading to the caller
         * @param learner if not nullptr, assume that the caller and reader read at a pace that isn't too fast for him
         */
        time_duration time_to_read( const item &book, const Character &reader,
                                    const Character *learner = nullptr ) const;

        /**
         * Helper function for get_book_reader
         *
         * @param book The book being read
         */
        read_condition_result check_read_condition( const item &book ) const;

        /** Calls Creature::normalize()
         *  nulls out the player's weapon
         *  Should only be called through player::normalize(), not on it's own!
         */
        void normalize() override;
        void die( Creature *nkiller ) override;
        virtual void prevent_death();

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

        // checks if your character is immune to an effect or field based on field_immunity_data
        bool check_immunity_data( const field_immunity_data &ft ) const override;
        bool is_immune_field( const field_type_id &fid ) const override;
        /** Returns true is the player is protected from electric shocks */
        bool is_elec_immune() const override;
        /** Returns true if the player is immune to this kind of effect */
        bool is_immune_effect( const efftype_id & ) const override;
        /** Returns true if the player is immune to this kind of damage */
        bool is_immune_damage( const damage_type_id & ) const override;
        /** Returns true if the player is protected from radiation */
        bool is_rad_immune() const;
        /** Returns true if the player is immune to knockdowns */
        bool is_knockdown_immune() const;
        /** Returns true if the player's melee skill increases the bash damage weapon cap */
        bool is_melee_bash_damage_cap_bonus() const;

        /**
         * Check if a given body part is immune to a given damage type
         *
         * This function checks whether a given body part cannot be damaged by a given
         * damage_unit.  Note that this refers only to reduction of hp on that part. It
         * does not account for clothing damage, pain, status effects, etc.
         *
         * @param bp: Body part to perform the check on
         * @param dam: Type of damage to check for
         * @returns true if given damage can not reduce hp of given body part
         */
        bool immune_to( const bodypart_id &bp, damage_unit dam ) const;

        /** Modifies a pain value by player traits before passing it to Creature::mod_pain() */
        void mod_pain( int npain ) override;
        /** Sets new intensity of pain an reacts to it */
        void set_pain( int npain ) override;
        /** Returns perceived pain (reduced with painkillers)*/
        int get_perceived_pain() const override;
        /** Returns multiplier on fall damage at low velocity (knockback/pit/1 z-level, not 5 z-levels) */
        float fall_damage_mod() const override;
        /** Deals falling/collision damage with terrain/creature at pos */
        int impact( int force, const tripoint &pos ) override;
        /** Knocks the player to a specified tile */
        void knock_back_to( const tripoint &to ) override;

        /** Returns overall % of HP remaining */
        int hp_percentage() const override;

        /** Returns true if the player has some form of night vision */
        bool has_nv();

        int get_lift_assist() const;

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

        std::string mutation_name( const trait_id &mut ) const;
        std::string mutation_desc( const trait_id &mut ) const;
        // In newcharacter.cpp
        void empty_skills();
        /** Returns a random name from NAMES_* */
        void pick_name( bool bUseDefault = false );
        /** Get the idents of all base traits. */
        std::vector<trait_id> get_base_traits() const;
        /** Get the idents of all traits/mutations. */
        std::vector<trait_id> get_mutations( bool include_hidden = true,
                                             bool ignore_enchantment = false ) const;
        /** Same as above, but also grab the variant ids (or empty string if none) */
        std::vector<trait_and_var> get_mutations_variants( bool include_hidden = true,
                bool ignore_enchantment = false ) const;
        const std::bitset<NUM_VISION_MODES> &get_vision_modes() const {
            return vision_mode_cache;
        }
        /** Empties the trait and mutations lists */
        void clear_mutations();
        /** Steps through the dependency chain for the given trait */
        void toggle_trait_deps( const trait_id &tr, const std::string &variant = "" );
        /**
         * Adds mandatory scenario and profession traits unless you already have them
         * And if you do already have them, refunds the points for the trait
         */
        void add_traits();
        /** Returns true if the player has crossed a mutation threshold
         *  Player can only cross one mutation threshold.
         */
        bool crossed_threshold() const;
        /** Returns the category that the player has crossed the threshold of, if they have one
         */
        mutation_category_id get_threshold_category() const;

        void environmental_revert_effect();

        /**
         * Checks both the neighborhoods of from and to for climbable surfaces,
         * returns move cost of climbing from `from` to `to`.
         * 0 means climbing is not possible.
         * Return value can depend on the orientation of the terrain.
         */
        int climbing_cost( const tripoint &from, const tripoint &to ) const;

        /** Which body part has the most staunchable bleeding, and what is the max improvement */
        bodypart_id most_staunchable_bp();
        bodypart_id most_staunchable_bp( int &max );

        void pause(); // '.' command; pauses & resets recoil

        /** Check player strong enough to lift an object unaided by equipment (jacks, levers etc) */
        template <typename T> bool can_lift( const T &obj ) const;
        // --------------- Values ---------------
        std::string name; // Pre-cataclysm name, invariable
        // In-game name which you give to npcs or whoever asks, variable
        std::optional<std::string> play_name;
        bool male = false;

        std::vector<effect_on_condition_id> death_eocs;
        outfit worn;
        bool nv_cached = false;
        // Means player sit inside vehicle on the tile he is now
        bool in_vehicle = false;
        bool hauling = false;
        tripoint view_offset;

        player_activity stashed_outbounds_activity;
        player_activity stashed_outbounds_backlog;
        player_activity activity;
        std::list<player_activity> backlog;
        std::optional<tripoint> destination_point;
        pimpl<inventory> inv;
        itype_id last_item;
    private:
        item weapon;
    public:
        item_location get_wielded_item() const;
        item_location get_wielded_item();
        // This invalidates the item_location returned by get_wielded_item
        void set_wielded_item( const item &to_wield );

        int scent = 0;
        pimpl<bionic_collection> my_bionics;
        pimpl<character_martial_arts> martial_arts_data;
        std::vector<matype_id> known_styles( bool teachable_only ) const;
        bool has_martialart( const matype_id &m ) const;

        stomach_contents stomach;
        stomach_contents guts;
        std::list<consumption_event> consumption_history;
        ///\EFFECT_STR increases breath-holding capacity while sinking or suffocating
        int get_oxygen_max() const;
        // Whether the character can recover their oxygen level
        bool can_recover_oxygen() const;
        int oxygen = 0;
        int slow_rad = 0;
        blood_type my_blood_type;
        bool blood_rh_factor = false;
        // Randomizes characters' blood type and Rh
        void randomize_blood();

        int avg_nat_bpm;
        void randomize_heartrate();

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
        std::set<mtype_id> known_monsters;
    public:
        int cash = 0;
        weak_ptr_fast<Creature> last_target;
        std::optional<tripoint> last_target_pos;
        // Save favorite ammo location
        item_location ammo_location;
        std::set<tripoint_abs_omt> camps;

        std::vector <addiction> addictions;
        std::vector<effect_on_condition_id> inactive_effect_on_condition_vector;
        queued_eocs queued_effect_on_conditions;

        /** Queue an EOC to add effect after a delay */
        void queue_effect( const std::string &name, const time_duration &delay,
                           const time_duration &duration );
        /** Count queued add_effect EOCs for specific effect */
        int count_queued_effects( const std::string &effect ) const;

        /** Adds an addiction to the player */
        void add_addiction( const addiction_id &type, int strength );
        /** Removes an addition from the player */
        void rem_addiction( const addiction_id &type );
        /** Returns true if the player has an addiction of the specified type */
        bool has_addiction( const addiction_id &type ) const;
        /** Returns the intensity of the specified addiction */
        int addiction_level( const addiction_id &type ) const;

        /** Returns true if the character is familiar with the given creature type **/
        bool knows_creature_type( const Creature *c ) const;
        /** Returns true if the character is familiar with the given creature type **/
        bool knows_creature_type( const mtype_id &c ) const;
        /** This character becomes familiar with creatures of the given type **/
        void set_knows_creature_type( const Creature *c );
        /** This character becomes familiar with creatures of the given type **/
        void set_knows_creature_type( const mtype_id &c );
        /** Returns a list of all monster types known by this character **/
        const std::set<mtype_id> &get_known_monsters() const {
            return known_monsters;
        }

        shared_ptr_fast<monster> mounted_creature;
        // for loading NPC mounts
        int mounted_creature_id = 0;
        // for vehicle work
        int activity_vehicle_part_index = -1;

        // Hauling items on the ground
        void start_hauling();
        void stop_hauling();
        bool is_hauling() const;

        /**
         * @brief Applies a lambda function on all items with the given flag and/or that pass the given boolean item function, using or creating caches from @ref inv_search_caches.
         * @param type Only process on items of this item type.
         * @param type_flag Only process on items whose type has this flag.
         * @param key A string to use as the key in the cache. Should usually be the same name as filter_func.
         * @param filter_func Only process on items that return true with this boolean item function. This is cached, so avoid using functions with varying results.
         * @param do_func A lambda function to apply on all items that pass the filters.
         */
        void cache_visit_items_with( const itype_id &type,
                                     const std::function<void( item & )> &do_func );
        void cache_visit_items_with( const flag_id &type_flag,
                                     const std::function<void( item & )> &do_func );
        void cache_visit_items_with( const std::string &key, bool( item::*filter_func )() const,
                                     const std::function<void( item & )> &do_func );
        void cache_visit_items_with( const std::string &key, const itype_id &type,
                                     const flag_id &type_flag, bool( item::*filter_func )() const,
                                     const std::function<void( item & )> &do_func );
        void cache_visit_items_with( const itype_id &type,
                                     const std::function<void( const item & )> &do_func ) const;
        void cache_visit_items_with( const flag_id &type_flag,
                                     const std::function<void( const item & )> &do_func ) const;
        void cache_visit_items_with( const std::string &key, bool( item::*filter_func )() const,
                                     const std::function<void( const item & )> &do_func ) const;
        void cache_visit_items_with( const std::string &key, const itype_id &type,
                                     const flag_id &type_flag, bool( item::*filter_func )() const,
                                     const std::function<void( const item & )> &do_func ) const;
        /**
        * @brief Returns true if the character has an item with given flag and/or that passes the given boolean item function, using or creating caches from @ref inv_search_caches.
        * @brief If you want to iterate over the entire cache, `cache_visit_items_with` should be used instead, as it's more optimized for processing entire caches.
        * @param type Look for items of this item type.
        * @param type_flag Look for items whose type has this flag.
        * @param key A string to use as the cache's key. Should usually be the same name as filter_func. Unneeded if checking only for a flag.
        * @param filter_func Look for items that return true with this boolean item function.
        * @param check_func An optional lambda function applied to items that pass the flag/bool function filters.
        * If it returns true, abort iteration and make cache_has_item_with return true. These results are not cached, unlike filter_func.
        * @return True if the character has an item that passes the criteria, including check_func if provided.
        */
        bool cache_has_item_with( const itype_id &type,
                                  const std::function<bool( const item & )> &check_func = return_true<item> ) const;
        bool cache_has_item_with( const flag_id &type_flag,
                                  const std::function<bool( const item & )> &check_func = return_true<item> ) const;
        bool cache_has_item_with( const std::string &key, bool( item::*filter_func )() const,
                                  const std::function<bool( const item & )> &check_func = return_true<item> ) const;
        bool cache_has_item_with( const std::string &key, const itype_id &type, const flag_id &type_flag,
                                  bool( item::*filter_func )() const,
                                  const std::function<bool( const item & )> &check_func = return_true<item> ) const;
        /**
        * @brief Find if the character has an item with a specific flag. Can also checks for charges, if needed.
        */
        bool has_item_with_flag( const flag_id &flag, bool need_charges = false ) const;
        /**
        * @brief Find if the character has an item whose type has a specific flag. Can also checks for charges, if needed. Uses or creates caches from @ref inv_search_caches.
        */
        bool cache_has_item_with_flag( const flag_id &type_flag, bool need_charges = false ) const;
        /**
         * @brief Returns all items with the given flag and/or that pass the given boolean item function, using or creating caches from @ref inv_search_caches.
         * @param type Only get items of this item type.
         * @param type_flag Only get items whose type has this flag.
         * @param key A string to use as the cache's key. Should usually be the same name as filter_func. Unneeded if checking only for a flag.
         * @param filter_func Only get items that return true with this boolean item function.
         * @param do_and_check_func An optional lambda function applied to items that pass the flag/bool function filters.
         * If it returns true, the item is added to the return vector. These results are not cached, unlike filter_func.
         * @return A vector of pointers to all items that pass the criteria.
         */
        std::vector<item *> cache_get_items_with( const itype_id &type,
                const std::function<bool( item & )> &do_and_check_func = return_true<item> );
        std::vector<item *> cache_get_items_with( const flag_id &type_flag,
                const std::function<bool( item & )> &do_and_check_func = return_true<item> );
        std::vector<item *> cache_get_items_with( const std::string &key,
                bool( item::*filter_func )() const,
                const std::function<bool( item & )> &do_and_check_func = return_true<item> );
        std::vector<item *> cache_get_items_with( const std::string &key, const itype_id &type,
                const flag_id &type_flag, bool( item::*filter_func )() const,
                const std::function<bool( item & )> &do_and_check_func = return_true<item> );
        std::vector<const item *> cache_get_items_with( const itype_id &type,
                const std::function<bool( const item & )> &check_func = return_true<item> ) const;
        std::vector<const item *> cache_get_items_with( const flag_id &type_flag,
                const std::function<bool( const item & )> &check_func = return_true<item> ) const;
        std::vector<const item *> cache_get_items_with( const std::string &key,
                bool( item::*filter_func )() const,
                const std::function<bool( const item & )> &check_func = return_true<item> ) const;
        std::vector<const item *> cache_get_items_with( const std::string &key, const itype_id &type,
                const flag_id &type_flag, bool( item::*filter_func )() const,
                const std::function<bool( const item & )> &check_func = return_true<item> ) const;
        /**
         * Add an item to existing @ref inv_search_caches that it meets the criteria for. Will NOT create any new caches.
         */
        void add_to_inv_search_caches( item &it ) const;

        bool has_charges( const itype_id &it, int quantity,
                          const std::function<bool( const item & )> &filter = return_true<item> ) const override;

        // has_amount works ONLY for quantity.
        // has_charges works ONLY for charges.
        std::list<item> use_amount( const itype_id &it, int quantity,
                                    const std::function<bool( const item & )> &filter = return_true<item>, bool select_ind = false );
        // Uses up charges
        bool use_charges_if_avail( const itype_id &it, int quantity );

        /**
        * Available ups from all sources
        * Sum of mech, bionic UPS and UPS
        * @return amount of UPS available
        */
        units::energy available_ups() const;

        /**
        * Consume UPS charges.
        * Consume order: mech, Bionic UPS, UPS.
        * @param qty amount of energy to consume. Is rounded down to kJ precision. Do not use negative values.
        * @return Actual amount of energy consumed
        */
        units::energy consume_ups( units::energy qty, int radius = -1 );

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
                                     const std::function<bool( const item & )> &filter = return_true<item>,
                                     bool in_tools = false );

        item find_firestarter_with_charges( int quantity ) const;
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
        /** Assigns activity to player, possibly resuming old activity if it's defined resumable. */
        void assign_activity( const player_activity &act );
        /** Assigns activity actor to player, possibly resuming old activity if it's defined resumable. */
        void assign_activity( const activity_actor &actor );
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
        bool can_stash( const item &it, bool ignore_pkt_settings = false );
        bool can_stash( const item &it, int &copies_remaining, bool ignore_pkt_settings = false );
        bool can_stash_partial( const item &it, bool ignore_pkt_settings = false );
        void initialize_stomach_contents();

        /** Stable base metabolic rate due to traits */
        float metabolic_rate_base() const;
        /** Current metabolic rate due to traits, hunger, speed, etc. */
        float metabolic_rate() const;
        // gets the max value healthy you can be, related to your weight
        int get_max_healthy() const;
        // calculates the BMI, either as the entire BMI or the BMI with only fat or lean mass accounted
        float get_bmi() const;
        float get_bmi_fat() const;
        float get_bmi_lean() const;
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
        int age( time_point when = calendar::turn ) const;
        std::string age_string( time_point when = calendar::turn ) const;
        // returns the height in cm
        int base_height() const;
        void set_base_height( int height );
        void mod_base_height( int mod );
        std::string height_string() const;
        // returns the height of the player character in cm
        int height() const;
        // Randomizes characters' height
        void randomize_height();
        // returns bodyweight of the Character
        units::mass bodyweight() const;
        // returns the weight of the player that is not fat (muscle, etc) based on height and strength
        units::mass bodyweight_lean() const;
        // returns the weight of the character that is fatty tissue based on stored kcal
        units::mass bodyweight_fat() const;
        // returns ratio of fat mass to lean total mass (unused currently)
        float fat_ratio() const;
        // returns total weight of installed bionics
        units::mass bionics_weight() const;
        // increases the activity level to the specified level
        // does not decrease activity level
        void set_activity_level( float new_level );
        // decreases activity level to the specified level
        // does not increase activity level
        void decrease_activity_level( float new_level );
        // sets activity level to NO_EXERCISE
        void reset_activity_level();
        // outputs player activity level to a printable string
        std::string activity_level_str() const;

        /** Returns overall env_resist on a body_part */
        int get_env_resist( bodypart_id bp ) const override;
        /** Returns overall resistance to given type on the bod part */
        int get_armor_type( const damage_type_id &dt, bodypart_id bp ) const override;
        std::map<bodypart_id, int> get_all_armor_type( const damage_type_id &dt,
                const std::map<bodypart_id, std::vector<const item *>> &clothing_map ) const;

        int get_stim() const;
        void set_stim( int new_stim );
        void mod_stim( int mod );

        int get_rad() const;
        void set_rad( int new_rad );
        void mod_rad( int mod );

        float get_heartrate_index() const;
        void update_heartrate_index();

        float get_bloodvol_index() const;
        void update_bloodvol_index();

        float get_circulation_resistance() const;
        void set_circulation_resistance( float ncirculation_resistance );

        void update_circulation();

        int get_stamina() const;
        int get_stamina_max() const;
        void set_stamina( int new_stamina );
        void mod_stamina( int mod );
        void burn_move_stamina( int moves );
        /** Regenerates stamina */
        void update_stamina( int turns );

        int get_cardiofit() const;

        int get_cardio_acc() const;
        void set_cardio_acc( int ncardio_acc );
        void reset_cardio_acc();
        int get_cardio_acc_base() const;
        virtual void update_cardio_acc() = 0;

        /** Returns true if a gun misfires, jams, or has other problems, else returns false */
        bool handle_gun_damage( item &it );

        /**Handles overheat mechanics for guns, returns true if the gun cant fire due to overheat effects*/
        bool handle_gun_overheat( item &it );

        /** Get maximum recoil penalty due to vehicle motion */
        double recoil_vehicle() const;

        /** Current total maximum recoil penalty from all sources */
        double recoil_total() const;

        /** How many moves does it take to aim gun to the target accuracy. */
        int gun_engagement_moves( const item &gun, int target = 0, int start = MAX_RECOIL,
                                  const Target_attributes &attributes = Target_attributes() ) const;

        /**
         *  Fires a gun or auxiliary gunmod (ignoring any current mode)
         *  @param target where the first shot is aimed at (may vary for later shots)
         *  @param shots maximum number of shots to fire (less may be fired in some circumstances)
         *  @return number of shots actually fired
         */

        int fire_gun( const tripoint &target, int shots = 1 );
        /**
         *  Fires a gun or auxiliary gunmod (ignoring any current mode)
         *  @param target where the first shot is aimed at (may vary for later shots)
         *  @param shots maximum number of shots to fire (less may be fired in some circumstances)
         *  @param gun item to fire (which does not necessary have to be in the players possession)
         *  @return number of shots actually fired
         */
        int fire_gun( const tripoint &target, int shots, item &gun );
        /** Execute a throw */
        dealt_projectile_attack throw_item( const tripoint &target, const item &to_throw,
                                            const std::optional<tripoint> &blind_throw_from_pos = std::nullopt );

    protected:
        void on_damage_of_type( const effect_source &source, int adjusted_damage,
                                const damage_type_id &dt, const bodypart_id &bp ) override;
    public:
        /** Called when an item is worn */
        void on_item_wear( const item &it );
        /** Called when an item is taken off */
        void on_item_takeoff( const item &it );
        // things to call when mutations enchantments change
        void enchantment_wear_change();
        /** Called when an item is washed */
        void on_worn_item_washed( const item &it );
        /** Called when an item is acquired (picked up, worn, or wielded) */
        void on_item_acquire( const item &it );
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
        /** Returns traits that cancel the entered trait */
        std::unordered_set<trait_id> get_opposite_traits( const trait_id &flag ) const;
        /** Removes "sleep" and "lying_down" */
        virtual void wake_up();
        // how loud a character can shout. based on mutations and clothing
        int get_shout_volume() const;
        // shouts a message
        void shout( std::string msg = "", bool order = false );
        //signals player location to nemesis for "Hunted" trait
        void signal_nemesis();
        /** Handles Character vomiting effects */
        void vomit();

        void customize_appearance( customize_appearance_choice choice );

        std::map<mutation_category_id, int> mutation_category_level;

        float adjust_for_focus( float amount ) const;
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

        /** Monster cameras are mtype_ids with an integer range of transmission */
        void clear_moncams();
        void remove_moncam( const mtype_id &moncam_id );
        void add_moncam( const std::pair<mtype_id, int> &moncam );
        void set_moncams( std::map<mtype_id, int> nmoncams );
        std::map<mtype_id, int> const &get_moncams() const;
        using cached_moncam = std::pair<monster const *, tripoint_abs_ms>;
        using moncam_cache_t = cata::flat_set<cached_moncam>;
        moncam_cache_t moncam_cache;
        moncam_cache_t get_active_moncams() const;

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

        ret_val<crush_tool_type> can_crush_frozen_liquid( item_location const &loc ) const;
        /** Prompts user about crushing item at item_location loc, for harvesting of frozen liquids
        * @param loc Location for item to crush */
        bool crush_frozen_liquid( item_location loc );

        float power_rating() const override;
        float speed_rating() const override;

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

        /** Modifies the movement cost and returns a list of effects */
        std::vector<run_cost_effect> run_cost_effects( float &movecost ) const;
        /** Returns the player's modified base movement cost */
        int run_cost( int base_cost, bool diag = false ) const;

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
        /** Returns the mutation visibility threshold for the observer ( *this ) */
        int get_mutation_visibility_cap( const Character *observed ) const;
        /** Returns an enumeration of visible mutations with colors */
        std::string visible_mutations( int visibility_cap ) const;

        player_activity get_destination_activity() const;
        void set_destination_activity( const player_activity &new_destination_activity );
        void clear_destination_activity();

        bool can_use_pockets() const;
        bool can_use_hood() const;
        bool can_use_collar() const;
        /** Returns warmth provided by an armor's bonus, like hoods, pockets, etc. */
        std::map<bodypart_id, int> bonus_item_warmth() const;
        /** Can the player lie down and cover self with blankets etc. **/
        bool can_use_floor_warmth() const;
        /**
         * Warmth from terrain, furniture, vehicle furniture and traps.
         * Can be negative.
         **/
        static units::temperature_delta floor_bedding_warmth( const tripoint &pos );
        /** Warmth from clothing on the floor **/
        static units::temperature_delta floor_item_warmth( const tripoint &pos );
        /** Final warmth from the floor **/
        units::temperature_delta floor_warmth( const tripoint &pos ) const;

        /** Correction factor of the body temperature due to traits and mutations **/
        units::temperature_delta bodytemp_modifier_traits( bool overheated ) const;
        /** Correction factor of the body temperature due to traits and mutations for player lying on the floor **/
        units::temperature_delta bodytemp_modifier_traits_floor() const;
        /** Value of the body temperature corrected by climate control **/
        units::temperature temp_corrected_by_climate_control( units::temperature temperature,
                int heat_strength,
                int chill_strength ) const;

        bool in_sleep_state() const override;
        /** Set vitamin deficiency/excess disease states dependent upon current vitamin levels */
        void update_vitamins( const vitamin_id &vit );
        /**
         * @brief Check current level of a vitamin
         *
         * @details Accesses level of a given vitamin.  If the vitamin_id specified does not
         * exist then this function simply returns 0.
         *
         * @param vit ID of vitamin to check level for (ie "vitC", "iron").
         * @returns character's current level for specified vitamin
         */
        int vitamin_get( const vitamin_id &vit ) const;
        // same as above, if actual = true get real daily intake, otherwise get speculated daily intake
        int get_daily_vitamin( const vitamin_id &vit, bool actual = true ) const;
        /**
         * Sets level of a vitamin
         *
         * @note status effects are still set for deficiency/excess
         *
         * @param[in] vit ID of vitamin to adjust quantity for
         * @param[in] qty Quantity to set level to
         */
        void vitamin_set( const vitamin_id &vit, int qty );
        /**
          * Add or subtract vitamins from character storage pools
         * @param vit ID of vitamin to modify
         * @param qty amount by which to adjust vitamin (negative values are permitted)
         * @return adjusted level for the vitamin or zero if vitamin does not exist
         */
        int vitamin_mod( const vitamin_id &vit, int qty );
        void vitamins_mod( const std::map<vitamin_id, int> & );
        /** Get vitamin usage rate (minutes per unit) accounting for bionics, mutations and effects */
        time_duration vitamin_rate( const vitamin_id &vit ) const;
        /** Modify vitamin intake (e.g. due to effects) */
        std::map<vitamin_id, int> effect_vitamin_mod( const std::map<vitamin_id, int> & );
        /** Remove all vitamins */
        void clear_vitamins();

        // resets the count for the given vitamin
        void reset_daily_vitamin( const vitamin_id &vit );

        /** Handles the nutrition value for a comestible **/
        int nutrition_for( const item &comest ) const;
        /** Can the food be [theoretically] eaten no matter the consequences? */
        ret_val<edible_rating> can_eat( const item &food ) const;
        /**
         * Same as @ref can_eat, but takes consequences into account.
         * Asks about them if @param interactive is true, refuses otherwise.
         */
        ret_val<edible_rating> will_eat( const item &food, bool interactive = false ) const;
        /** Determine character's capability of recharging their CBMs.
        * Returns energy in kJ
        */
        int get_acquirable_energy( const item &it ) const;

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
        /** Handles the effects of consuming an item. Returns false if nothing was consumed */
        bool consume_effects( item &food );
        /** Check whether the character can consume this very item */
        bool can_consume_as_is( const item &it ) const;
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
            std::map<recipe_id, std::pair<nutrients, nutrients>> &rec_cache,
            const cata::flat_set<flag_id> &extra_flags = {} ) const;
        /** Same, but across arbitrary recipes */
        std::pair<nutrients, nutrients> compute_nutrient_range(
            const itype_id &,
            std::map<recipe_id, std::pair<nutrients, nutrients>> &rec_cache,
            const cata::flat_set<flag_id> &extra_flags = {} ) const;
        /** Returns allergy type or MORALE_NULL if not allergic for this character */
        morale_type allergy_type( const item &food ) const;
        nutrients compute_effective_nutrients( const item & ) const;
        /** Returns true if the character is wearing something on the entered body part. Ignores INTEGRATED */
        bool wearing_something_on( const bodypart_id &bp ) const;
        /** Returns true if the character is wearing something on the entered body part. Ignores INTEGRATED and OVERSIZE */
        bool wearing_fitting_on( const bodypart_id &bp ) const;
        /** Same as footwear factor, but for arms */
        double armwear_factor() const;
        /** Returns 1 if the player is wearing an item of that count on one foot, 2 if on both,
         *  and zero if on neither */
        int shoe_type_count( const itype_id &it ) const;
        /** Returns true if the player is wearing something on their feet that is not SKINTIGHT */
        bool is_wearing_shoes( const side &check_side = side::BOTH ) const;

        /** Returns true if the player is not wearing anything that covers the soles of their feet,
            ignoring INTEGRATED */
        bool is_barefoot() const;

        /** Returns true if the worn item is visible (based on layering and coverage) */
        bool is_worn_item_visible( std::list<item>::const_iterator ) const;

        /** Returns all worn items visible to an outside observer */
        std::list<item> get_visible_worn_items() const;

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
        const inventory &crafting_inventory( bool clear_path ) const;
        /**
        * Returns items that can be used to craft with. Always includes character inventory.
        * @param src_pos Character position.
        * @param radius Radius from src_pos. -1 to return items in character inventory only.
        * @param clear_path True to select only items within view. False to select all within the radius.
        * @returns Craftable inventory items found.
        * */
        const inventory &crafting_inventory( const tripoint &src_pos = tripoint_zero,
                                             int radius = PICKUP_RANGE, bool clear_path = true ) const;
        void invalidate_crafting_inventory();

        /** Returns a value from 1.0 to 11.0 that acts as a multiplier
         * for the time taken to perform tasks that require detail vision,
         * above 4.0 means these activities cannot be performed.
         * takes pos as a parameter so that remote spots can be judged
         * if they will potentially have enough light when player gets there */
        float fine_detail_vision_mod( const tripoint &p = tripoint_min ) const;

        // ---- CRAFTING ----
        void make_craft_with_command( const recipe_id &id_to_make, int batch_size, bool is_long,
                                      const std::optional<tripoint> &loc );
        pimpl<craft_command> last_craft;

        recipe_id lastrecipe;
        int last_batch;

        // Returns true if the character knows the recipe, is near a book or device
        // providing the recipe or a nearby Character knows it.
        bool has_recipe( const recipe *r, const inventory &crafting_inv,
                         const std::vector<Character *> &helpers ) const;
        bool knows_recipe( const recipe *rec ) const;
        void learn_recipe( const recipe *rec );
        void forget_recipe( const recipe *rec );
        void forget_all_recipes();
        int exceeds_recipe_requirements( const recipe &rec ) const;
        bool has_recipe_requirements( const recipe &rec ) const;
        bool can_decomp_learn( const recipe &rec ) const;

        bool studied_all_recipes( const itype &book ) const;

        /** Returns all known recipes. */
        const recipe_subset &get_learned_recipes() const;
        recipe_subset get_available_nested( const recipe_subset & ) const;
        /** Returns all recipes that are known from the books (either in inventory or nearby). */
        recipe_subset get_recipes_from_books( const inventory &crafting_inv ) const;
        /** Returns all recipes that are known from the books inside ereaders (either in inventory or nearby). */
        recipe_subset get_recipes_from_ebooks( const inventory &crafting_inv ) const;
        /**
          * Return all available recipes (from books and companions)
          * @param crafting_inv Current available items to craft
          * @param helpers List of Characters that could help with crafting.
          */
        recipe_subset get_available_recipes( const inventory &crafting_inv,
                                             const std::vector<Character *> *helpers = nullptr ) const;
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
        float lighting_craft_speed_multiplier( const recipe &rec, const tripoint &p = tripoint_min ) const;
        float crafting_speed_multiplier( const recipe &rec ) const;
        float workbench_crafting_speed_multiplier( const item &craft,
                const std::optional<tripoint> &loc )const;
        /** For use with in progress crafts.
         *  Workbench multiplier calculation (especially finding lifters nearby)
         *  is expensive when numorous items are around.
         *  So use pre-calculated cache if possible.
         */
        float crafting_speed_multiplier( const item &craft, const std::optional<tripoint> &loc,
                                         bool use_cached_workbench_multiplier = false, float cached_workbench_multiplier = 0.0f
                                       ) const;
        int available_assistant_count( const recipe &rec ) const;
        /**
         * Expected time to craft a recipe, with assumption that multipliers stay constant.
         */
        int64_t expected_time_to_craft( const recipe &rec, int batch_size = 1 ) const;
        std::vector<const item *> get_eligible_containers_for_crafting() const;
        bool check_eligible_containers_for_crafting( const recipe &rec, int batch_size = 1 ) const;
        bool can_make( const recipe *r, int batch_size = 1 ) const;  // have components?
        /**
         * Returns true if the player can start crafting the recipe with the given batch size
         * The player is not required to have enough tool charges to finish crafting, only to
         * complete the first step (total / 20 + total % 20 charges)
         */
        bool can_start_craft( const recipe *rec, recipe_filter_flags, int batch_size = 1 ) const;
        bool making_would_work( const recipe_id &id_to_make, int batch_size ) const;

        /**
         * Start various types of crafts
         * @param loc the location of the workbench. std::nullopt indicates crafting from inventory.
         * @param goto_recipe the recipe to display initially. A null recipe_id opens the default crafting screen.
         */
        void craft( const std::optional<tripoint> &loc = std::nullopt,
                    const recipe_id &goto_recipe = recipe_id(), const std::string &filterstring = "" );
        void recraft( const std::optional<tripoint> &loc = std::nullopt );
        void long_craft( const std::optional<tripoint> &loc = std::nullopt,
                         const recipe_id &goto_recipe = recipe_id() );
        void make_craft( const recipe_id &id, int batch_size,
                         const std::optional<tripoint> &loc = std::nullopt );
        void make_all_craft( const recipe_id &id, int batch_size,
                             const std::optional<tripoint> &loc );
        /** consume components and create an active, in progress craft containing them */
        void start_craft( craft_command &command, const std::optional<tripoint> &loc );

        struct craft_roll_data {
            float center;
            float stddev;
            float final_difficulty;
        };
        /**
         * Calculate a value representing the success of the player at crafting the given recipe,
         * taking player skill, recipe difficulty, npc helpers, and player mutations into account.
         * @param making the recipe for which to calculate
         * @return a value >= 0.0 with >= 1.0 representing unequivocal success
         */
        float crafting_success_roll( const recipe &making ) const;
        float crafting_failure_roll( const recipe &making ) const;
        float get_recipe_weighted_skill_average( const recipe &making ) const;
        float recipe_success_chance( const recipe &making ) const;
        float item_destruction_chance( const recipe &making ) const;
        craft_roll_data recipe_success_roll_data( const recipe &making ) const;
        craft_roll_data recipe_failure_roll_data( const recipe &making ) const;
        void complete_craft( item &craft, const std::optional<tripoint> &loc );
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
        /** Return nearby Characters ready and willing to help with crafting. */
        std::vector<Character *> get_crafting_helpers() const;
        /**
         * Return group of ally characters sharing knowledge of crafting.
         * Unlike get_crafting_helpers, includes the caller and sleeping Characters.
         */
        std::vector<Character *> get_crafting_group() const;
        int get_num_crafting_helpers( int max ) const;
        /**
         * Handle skill gain for player and followers during crafting.
         * Returns true if character leveled up.
         * @param craft the currently in progress craft
         * @param num_practice_ticks to trigger.  This is used to apply
         * multiple steps of incremental skill gain simultaneously if needed.
         */
        bool craft_skill_gain( const item &craft, const int &num_practice_ticks );
        /**
         * Handle proficiency practice for player and followers while crafting. Returns
         * true if a proficiency is acquired.
         * @param craft - the in progress craft
         * @param time - the amount of time since the last practice tick
         */
        bool craft_proficiency_gain( const item &craft, const time_duration &time );
        /**
         * Check if the player can disassemble an item using the current crafting inventory
         * @param obj Object to check for disassembly
         * @param inv current crafting inventory
         */
        ret_val<void> can_disassemble( const item &obj, const read_only_visitable &inv ) const;
        item_location create_in_progress_disassembly( item_location target );

        bool disassemble();
        bool disassemble( item_location target, bool interactive = true, bool disassemble_all = false );
        void disassemble_all( bool one_pass ); // Disassemble all items on the tile
        /**
         * Completely disassemble an item, and drop yielded components at its former position.
         * @param target - the in-progress disassembly item location
         * @param dis - recipe for disassembly (by default uses recipe_dictionary::get_uncraft)
         */
        void complete_disassemble( item_location target );
        void complete_disassemble( item_location &target, const recipe &dis );

        const requirement_data *select_requirements(
            const std::vector<const requirement_data *> &, int batch, const read_only_visitable &,
            const std::function<bool( const item & )> &filter ) const;
        comp_selection<item_comp>
        select_item_component( const std::vector<item_comp> &components,
                               int batch, read_only_visitable &map_inv, bool can_cancel = false,
                               const std::function<bool( const item & )> &filter = return_true<item>, bool player_inv = true,
                               bool npc_query = false, const recipe *rec = nullptr );
        std::list<item> consume_items( const comp_selection<item_comp> &is, int batch,
                                       const std::function<bool( const item & )> &filter = return_true<item>, bool select_ind = false );
        std::list<item> consume_items( map &m, const comp_selection<item_comp> &is, int batch,
                                       const std::function<bool( const item & )> &filter = return_true<item>,
                                       const tripoint &origin = tripoint_zero, int radius = PICKUP_RANGE, bool select_ind = false );
        std::list<item> consume_items( map &m, const comp_selection<item_comp> &is, int batch,
                                       const std::function<bool( const item & )> &filter = return_true<item>,
                                       const std::vector<tripoint> &reachable_pts = {}, bool select_ind = false );
        std::list<item> consume_items( const std::vector<item_comp> &components, int batch = 1,
                                       const std::function<bool( const item & )> &filter = return_true<item>,
                                       const std::function<bool( const itype_id & )> &select_ind = return_false<itype_id> );
        bool consume_software_container( const itype_id &software_id );
        comp_selection<tool_comp>
        select_tool_component( const std::vector<tool_comp> &tools, int batch, read_only_visitable &map_inv,
                               bool can_cancel = false, bool player_inv = true, bool npc_query = false,
        const std::function<int( int )> &charges_required_modifier = []( int i ) {
            return i;
        } );
        /** Consume tools for the next multiplier * 5% progress of the craft */
        bool craft_consume_tools( item &craft, int multiplier, bool start_craft );
        void consume_tools( const comp_selection<tool_comp> &tool, int batch );
        void consume_tools( map &m, const comp_selection<tool_comp> &tool, int batch,
                            const tripoint &origin = tripoint_zero, int radius = PICKUP_RANGE,
                            basecamp *bcp = nullptr );
        void consume_tools( map &m, const comp_selection<tool_comp> &tool, int batch,
                            const std::vector<tripoint> &reachable_pts = {},   basecamp *bcp = nullptr );
        void consume_tools( const std::vector<tool_comp> &tools, int batch = 1 );

        /** Checks permanent morale for consistency and recovers it when an inconsistency is found. */
        void check_and_recover_morale();

        /**
         * Handles the enjoyability value for a comestible.
         *
         * If `ignore_already_ate`, fun isn't affected by past consumption.
         * Return First value is enjoyability, second is cap.
         */
        std::pair<int, int> fun_for( const item &comest, bool ignore_already_ate = false ) const;

        /** Handles a large number of timers decrementing and other randomized effects */
        void suffer();
        /** Handles mitigation and application of radiation */
        bool irradiate( float rads, bool bypass = false );
        /** Handles the chance for broken limbs to spontaneously heal to 1 HP */
        void mend( int rate_multiplier );

    private:
        // Amount of radiation (mSv) leaked from carried items.
        float leak_level = 0.0f;
        /** Signify that leak_level needs refreshing. Set to true on inventory change. */
        bool leak_level_dirty = true;
        // Cache if current bionic layout has certain json flag. Refreshed upon bionics add/remove, activation/deactivation.
        mutable std::map<const json_character_flag, bool> bio_flag_cache;
    public:
        float get_leak_level() const;
        /** Iterate through the character inventory to get its leak level */
        void calculate_leak_level();
        /** Sets leak_level_dirty to true */
        void invalidate_leak_level_cache();

        /** Creates an auditory hallucination */
        void sound_hallu();

        /** Checks if a Character is driving */
        bool is_driving() const;

        /** Drenches the player with water, saturation is the percent gotten wet */
        void drench( int saturation, const body_part_set &flags, bool ignore_waterproof );
        /** Recalculates morale penalty/bonus from wetness based on mutations, equipment and temperature */
        void apply_wetness_morale( units::temperature temperature );
        int heartrate_bpm() const;
        std::vector<std::string> short_description_parts() const;
        std::string short_description() const;
        // Checks whether a player can hear a sound at a given volume and location.
        bool can_hear( const tripoint &source, int volume ) const;
        // Returns a multiplier indicating the keenness of a player's hearing.
        float hearing_ability() const;

        using trap_map = std::map<tripoint, std::string>;
        // Use @ref trap::can_see to check whether a character knows about a
        // specific trap - it will consider visible and known traps.
        bool knows_trap( const tripoint &pos ) const;
        void add_known_trap( const tripoint &pos, const trap &t );

        // see Creature::sees
        bool sees( const tripoint &t, bool is_avatar = false, int range_mod = 0 ) const override;
        bool sees( const tripoint_bub_ms &t, bool is_avatar = false, int range_mod = 0 ) const override;
        // see Creature::sees
        bool sees( const Creature &critter ) const override;
        Attitude attitude_to( const Creature &other ) const override;
        virtual npc_attitude get_attitude() const;

        // used in debugging all health
        int get_lowest_hp() const;
        bool has_weapon() const override;
        void shift_destination( const point &shift );
        // Auto move methods
        void set_destination( const std::vector<tripoint_bub_ms> &route,
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
        std::vector<tripoint_bub_ms> &get_auto_move_route();
        action_id get_next_auto_move_direction();
        bool defer_move( const tripoint &next );
        time_duration get_consume_time( const item &it ) const;

        // For display purposes mainly, how far we are from the next level of weariness
        std::pair<int, int> weariness_transition_progress() const;
        int weariness_level() const;
        int weariness_transition_level() const;
        int weary_threshold() const;
        int weariness() const;
        float activity_level() const;
        /** Returns instantaneous activity level as a float from 0-10 (from game_constants) */
        float instantaneous_activity_level() const;
        /** Returns instantaneous activity level as an int from 0-5 (half of instantaneous_activity_level) */
        int activity_level_index() const;
        float exertion_adjusted_move_multiplier( float level = -1.0f ) const;
        float maximum_exertion_level() const;
        std::string activity_level_str( float level ) const;
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

        /** Estimate effect duration based on player relevant skill.
        @param error_magnitude Maximum error, with zero in the relevant skill.
        @param minimum_error Maximum error when skill is >= threshold */
        time_duration estimate_effect_dur( const skill_id &relevant_skill, const efftype_id &effect,
                                           const time_duration &error_magnitude,
                                           const time_duration &minimum_error, int threshold, const Creature &target ) const;

        /** Look for items in the player's inventory that have the specified quality; return the one with highest level
         * @param qual_id The quality to search
        */
        item &best_item_with_quality( const quality_id &qid );

        // inherited from visitable
        bool has_quality( const quality_id &qual, int level = 1, int qty = 1 ) const override;
        int max_quality( const quality_id &qual ) const override;
        int max_quality( const quality_id &qual, int radius ) const;
        VisitResponse visit_items( const std::function<VisitResponse( item *, item * )> &func ) const
        override;
        std::list<item> remove_items_with( const std::function<bool( const item & )> &filter,
                                           int count = INT_MAX ) override;
        int charges_of(
            const itype_id &what, int limit = INT_MAX,
            const std::function<bool( const item & )> &filter = return_true<item>,
            const std::function<void( int )> &visitor = nullptr,
            bool in_tools = false ) const override;
        int amount_of(
            const itype_id &what, bool pseudo = true, int limit = INT_MAX,
            const std::function<bool( const item & )> &filter = return_true<item> ) const override;

        std::pair<int, int> kcal_range(
            const itype_id &id,
            const std::function<bool( const item & )> &filter, Character &player_character ) const;

    protected:
        Character();
        Character( Character && ) noexcept( map_is_noexcept );
        Character &operator=( Character && ) noexcept( list_is_noexcept );
        void swap_character( Character &other );
    public:
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
            time_duration charge = 0_turns;

            bool show_sprite = true;

            /** A trait variant if it has one, or nullptr */
            const mutation_variant *variant = nullptr;

            void serialize( JsonOut &json ) const;
            void deserialize( const JsonObject &data );

            trait_data() = default;
            explicit trait_data( const mutation_variant *chosen ) : variant( chosen ) {}
        };

        /** Bonuses to stats, calculated each turn */
        int str_bonus = 0;
        int dex_bonus = 0;
        int per_bonus = 0;
        int int_bonus = 0;
        /** Hardcoded stats bonus */
        int str_bonus_hardcoded = 0;
        int dex_bonus_hardcoded = 0;
        int per_bonus_hardcoded = 0;
        int int_bonus_hardcoded = 0;
        // cached so the display knows how much your bonus is
        int enchantment_speed_bonus = 0;

        /** How healthy the character is. */
        int lifestyle = 0;
        int daily_health = 0;
        int health_tally = 0;

        // Our bmr at no activity level
        int base_bmr() const;

        /** age in years at character creation */
        int init_age = 25;
        /**height at character creation*/
        int init_height = 175;
        /** Size class of character. */
        creature_size size_class = creature_size::medium;

        /**
        * Min, max and default heights for each size category of a character in centimeters.
        * minimum is 2 std. deviations below average female height
        */
        /**@{*/
        static int min_height( creature_size size_category = creature_size::medium );
        static int default_height( creature_size size_category = creature_size::medium );
        static int max_height( creature_size size_category = creature_size::medium );
        /**@}*/

        // the player's activity level for metabolism calculations
        activity_tracker activity_history;

        // Our weariness level last turn, so we know when we transition
        int old_weary_level = 0;

        trap_map known_traps;
        mutable std::map<std::string, double> cached_info;
        bool bio_soporific_powered_at_last_sleep_check = false;
        /** last time we checked for sleep */
        time_point last_sleep_check = calendar::turn_zero;
        /** warnings from a faction about bad behavior */
        mutable std::map<faction_id, std::pair<int, time_point>> warning_record;
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

        // if the player puts on and takes off items these mutations
        // are added or removed at the beginning of the next
        std::vector<trait_id> mutations_to_remove;
        std::vector<trait_id> mutations_to_add;
        /**
         * The amount of weight the Character is carrying.
         * If it is nullopt, needs to be recalculated
         */
        mutable std::optional<units::mass> cached_weight_carried = std::nullopt;

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
        /** Current quantity for each vitamin today first value is expected second value is actual (digested) in vitamin units*/
        std::map<vitamin_id, std::pair<int, int>> daily_vitamins;
        /** Returns the % of your RDA that ammount of vitamin represents */
        int vitamin_RDA( const vitamin_id &vitamin, int ammount ) const;

        pimpl<player_morale> morale;
        /** Processes human-specific effects of an effect. */
        void process_one_effect( effect &it, bool is_new ) override;

        /**
         * Map body parts to their total exposure, from 0.0 (fully covered) to 1.0 (buck naked).
         * Clothing layers are multiplied, ex. two layers of 50% coverage will leave only 25% exposed.
         * Used to determine suffering effects of albinism and solar sensitivity.
         */
        std::map<bodypart_id, float> bodypart_exposure();
    private:
        /**
         * Check whether the other creature is in range and can be seen by this creature.
         * @param critter Creature to check for visibility
         * @param range The maximal distance (@ref rl_dist), creatures at this distance or less
         * are included.
         */
        bool is_visible_in_range( const Creature &critter, int range ) const;

        struct bionic_fuels;

        /**
         * Check fuel for the bionic.
         * Returns fuels available for the bionic.
         * @param bio the bionic
         * @param start true if player is trying to turn the bionic on
         */
        bionic_fuels bionic_fuel_check( bionic &bio, bool start );

        /**
         * Convert fuel to bionic power. Handles both active and passive bionics
         * @param bio the bionic
         */
        void burn_fuel( bionic &bio );

        player_activity destination_activity;
        /// A unique ID number, assigned by the game class. Values should never be reused.
        character_id id;

        units::energy power_level;
        units::energy max_power_level_cached;
        units::energy max_power_level_modifier;

        // Additional vision sources, currently only used by avatars
        std::map<mtype_id, int> moncams;

        /// @brief Needs (hunger, starvation, thirst, fatigue, etc.)
        // Stored calories is a value in 'calories' - 1/1000s of kcals (or Calories)
        int stored_calories;
        int healthy_calories;

        int hunger;
        int thirst;
        int stamina;

        int cardio_acc;
        int base_cardio_acc;

        // All indices represent the percentage compared to normal.
        // i.e. a value of 1.1 means 110% of normal.
        float heart_rate_index = 1.0f;
        float blood_vol_index = 1.0f;

        float circulation;
        // Should remain fixed at 1.0 for now.
        float circulation_resistance = 1.0f;

        time_duration daily_sleep = 0_turns;
        time_duration continuous_sleep = 0_turns;

        int fatigue;
        int sleep_deprivation;
        bool check_encumbrance = true;
        bool cache_inventory_is_valid = false;
        mutable bool using_lifting_assist = false;

        int stim;
        int pkill;

        int radiation;

        std::vector<tripoint_bub_ms> auto_move_route;
        // Used to make sure auto move is canceled if we stumble off course
        std::optional<tripoint_bub_ms> next_expected_position;
        scenttype_id type_of_scent;

        struct weighted_int_list<std::string> melee_miss_reasons;

        struct crafting_cache_type {
            bool valid = false; // other fields are only valid if this flag is true
            time_point time;
            int moves;
            tripoint position;
            int radius;
            pimpl<inventory> crafting_inventory;
        };
        mutable crafting_cache_type crafting_cache;

        time_point melee_warning_turn = calendar::turn_zero;

        mutable bool pseudo_items_valid = false;
        mutable std::vector<const item *> pseudo_items;

        /** Cached results for functions that search through the inventory, like @ref cache_visit_items_with */
        struct inv_search_cache {
            itype_id type;
            flag_id type_flag;
            bool ( item::*filter_func )() const;
            std::list<safe_reference<item>> items;
        };
        mutable std::unordered_map<std::string, inv_search_cache> inv_search_caches;
    protected:
        // Bionic IDs are unique only within a character. Used to unambiguously identify bionics in a character
        bionic_uid weapon_bionic_uid = 0;
        mutable bionic_uid next_bionic_uid = 0;  // NOLINT(cata-serialize)

        /** Subset of learned recipes. Needs to be mutable for lazy initialization. */
        mutable pimpl<recipe_subset> learned_recipes;

        /** Stamp of skills. @ref learned_recipes are valid only with this set of skills. */
        mutable decltype( _skills ) valid_autolearn_skills;

        /** Amount of time the player has spent in each overmap tile. */
        std::unordered_map<point_abs_omt, time_duration> overmap_time;

    public:
        time_point next_climate_control_check;
        bool last_climate_control_ret;

        // a cache of all active enchantment values.
        // is recalculated every turn in Character::recalculate_enchantment_cache
        pimpl<enchant_cache> enchantment_cache;
};

Character &get_player_character();

template<>
struct enum_traits<character_stat> {
    static constexpr character_stat last = character_stat::DUMMY_STAT;
};
/// Get translated name of a stat
std::string get_stat_name( character_stat Stat );

extern template bool Character::can_lift<item>( const item &obj ) const;
extern template bool Character::can_lift<vehicle>( const vehicle &obj ) const;
#endif // CATA_SRC_CHARACTER_H
