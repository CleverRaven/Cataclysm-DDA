#pragma once
#ifndef MUTATION_H
#define MUTATION_H

#include <map>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>
#include <memory>
#include <string>

#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "damage.h"
#include "string_id.h"
#include "hash_utils.h"
#include "translations.h"
#include "type_id.h"
#include "point.h"

class nc_color;
class JsonObject;
class player;
struct dream;
class Trait_group;
class item;

using itype_id = std::string;
class JsonArray;

extern std::vector<dream> dreams;
extern std::map<std::string, std::vector<trait_id> > mutations_category;

struct dream {
    private:
        std::vector<std::string> raw_messages; // The messages that the dream will give

    public:
        std::vector<std::string> messages() const;

        std::string category; // The category that will trigger the dream
        int strength; // The category strength required for the dream

        dream() {
            strength = 0;
        }

        static void load( JsonObject &jsobj );
};

struct mut_attack {
    /** Text printed when the attack is proced by you */
    std::string attack_text_u;
    /** As above, but for npc */
    std::string attack_text_npc;
    /** Need all of those to qualify for this attack */
    std::set<trait_id> required_mutations;
    /** Need none of those to qualify for this attack */
    std::set<trait_id> blocker_mutations;

    /** If not num_bp, this body part needs to be uncovered for the attack to proc */
    body_part bp = num_bp;

    /** Chance to proc is one_in( chance - dex - unarmed ) */
    int chance = 0;

    damage_instance base_damage;
    /** Multiplied by strength and added to the above */
    damage_instance strength_damage;

    /** Should be true when and only when this attack needs hardcoded handling */
    bool hardcoded_effect = false;
};

struct mutation_branch {
        trait_id id;
        bool was_loaded = false;
        // True if this is a valid mutation (False for "unavailable from generic mutagen").
        bool valid = false;
        // True if Purifier can remove it (False for *Special* mutations).
        bool purifiable;
        // True if it's a threshold itself, and shouldn't be obtained *easily* (False by default).
        bool threshold;
        // True if this is a trait associated with professional training/experience, so profession/quest ONLY.
        bool profession;
        //True if the mutation is obtained through the debug menu
        bool debug;
        // True if the mutation should be displayed in the `@` menu
        bool player_display = true;
        // Whether it has positive as well as negative effects.
        bool mixed_effect  = false;
        bool startingtrait = false;
        bool activated     = false;
        // Should it activate as soon as it is gained?
        bool starts_active = false;
        // Should it destroy gear on restricted body parts? (otherwise just pushes it off)
        bool destroys_gear = false;
        // Allow soft (fabric) gear on restricted body parts
        bool allow_soft_gear  = false;
        // IF any of the three are true, it drains that as the "cost"
        bool fatigue       = false;
        bool hunger        = false;
        bool thirst        = false;
        // How many points it costs in character creation
        int points     = 0;
        int visibility = 0;
        int ugliness   = 0;
        int cost       = 0;
        // costs are consumed every cooldown turns,
        int cooldown   = 0;
        // bodytemp elements:
        int bodytemp_min = 0;
        int bodytemp_max = 0;
        int bodytemp_sleep = 0;
        // Healing per turn
        float healing_awake = 0.0f;
        float healing_resting = 0.0f;
        // Bonus HP multiplier. That is, 1.0 doubles hp, -0.5 halves it.
        float hp_modifier = 0.0f;
        // Second HP modifier that stacks with first but is otherwise identical.
        float hp_modifier_secondary = 0.0f;
        // Flat bonus/penalty to hp.
        float hp_adjustment = 0.0f;
        // Modify strength stat without changing HP
        float str_modifier = 0.0f;
        // Additional bonuses
        float dodge_modifier = 0.0f;
        float speed_modifier = 1.0f;
        float movecost_modifier = 1.0f;
        float movecost_flatground_modifier = 1.0f;
        float movecost_obstacle_modifier = 1.0f;
        float attackcost_modifier = 1.0f;
        float max_stamina_modifier = 1.0f;
        float weight_capacity_modifier = 1.0f;
        float hearing_modifier = 1.0f;
        float noise_modifier = 1.0f;
        float scent_modifier = 1.0f;
        int bleed_resist = 0;

        /**Rate at which bmi above character_weight_category::normal increases the character max_hp*/
        float fat_to_max_hp = 0.0f;
        /**How fast does healthy tends toward healthy_mod*/
        float healthy_rate = 1.0f;

        /**maximum damage dealt by water every minute when wet. Can be negative and regen hit points.*/
        int weakness_to_water = 0;

        // Subtracted from the range at which monsters see player, corresponding to percentage of change. Clamped to +/- 60 for effectiveness
        float stealth_modifier = 0.0f;

        // Speed lowers--or raises--for every X F (X C) degrees below or above 65 F (18.3 C)
        float temperature_speed_modifier = 0.0f;
        // Extra metabolism rate multiplier. 1.0 doubles usage, -0.5 halves.
        float metabolism_modifier = 0.0f;
        // As above but for thirst.
        float thirst_modifier = 0.0f;
        // As above but for fatigue.
        float fatigue_modifier = 0.0f;
        // Modifier for the rate at which fatigue and sleep deprivation drops when resting.
        float fatigue_regen_modifier = 0.0f;
        // Modifier for the rate at which stamina regenerates.
        float stamina_regen_modifier = 0.0f;

        // Adjusts sight range on the overmap. Positives make it farther, negatives make it closer.
        float overmap_sight = 0.0f;

        // Multiplier for sight range, defaulting to 1.
        float overmap_multiplier = 1.0f;

        // Multiplier for map memory capacity, defaulting to 1.
        float map_memory_capacity_multiplier = 1.0f;

        // Multiplier for skill rust, defaulting to 1.
        float skill_rust_multiplier = 1.0f;

        // Bonus or penalty to social checks (additive).  50 adds 50% to success, -25 subtracts 25%
        social_modifiers social_mods;

        /** The item, if any, spawned by the mutation */
        itype_id spawn_item;

        /**Species ignoring character with the mutation*/
        std::vector<species_id> ignored_by;

        /**List of material required for food to be be edible*/
        std::set<material_id> can_only_eat;

        /**List of healing items allowed*/
        std::set<itype_id> can_only_heal_with;
        std::set<itype_id> can_heal_with;

        /**List of allowed mutatrion category*/
        std::set<std::string> allowed_category;

        /**List of body parts locked out of bionics*/
        std::set<body_part> no_cbm_on_bp;

        // amount of mana added or subtracted from max
        float mana_modifier;
        float mana_multiplier;
        float mana_regen_multiplier;
        // spells learned and their associated level when gaining the mutation
        std::map<spell_id, int> spells_learned;
    private:
        std::string raw_spawn_item_message;
    public:
        std::string spawn_item_message() const;

        /** The fake gun, if any, spawned and fired by the ranged mutation */
        itype_id ranged_mutation;
    private:
        std::string raw_ranged_mutation_message;
    public:
        std::string ranged_mutation_message() const;

        /** Attacks granted by this mutation */
        std::vector<mut_attack> attacks_granted;

        /** Mutations may adjust one or more of the default vitamin usage rates */
        std::map<vitamin_id, time_duration> vitamin_rates;

        // Mutations may affect absorption rates of vitamins based on material (or "all")
        std::map<material_id, std::map<vitamin_id, double>> vitamin_absorb_multi;

        std::vector<trait_id> prereqs; // Prerequisites; Only one is required
        std::vector<trait_id> prereqs2; // Prerequisites; need one from here too
        std::vector<trait_id> threshreq; // Prerequisites; dedicated slot to needing thresholds
        std::set<std::string> types; // Mutation types, you can't have two mutations that share a type
        std::vector<trait_id> cancels; // Mutations that conflict with this one
        std::vector<trait_id> replacements; // Mutations that replace this one
        std::vector<trait_id> additions; // Mutations that add to this one
        std::vector<std::string> category; // Mutation Categories
        std::set<std::string> flags; // Mutation flags
        std::map<body_part, tripoint> protection; // Mutation wet effects
        std::map<body_part, int> encumbrance_always; // Mutation encumbrance that always applies
        // Mutation encumbrance that applies when covered with unfitting item
        std::map<body_part, int> encumbrance_covered;
        // Body parts that now need OVERSIZE gear
        std::set<body_part> restricts_gear;
        // Mutation stat mods
        /** Key pair is <active: bool, mod type: "STR"> */
        std::unordered_map<std::pair<bool, std::string>, int, cata::tuple_hash> mods;
        std::map<body_part, resistances> armor;
        std::vector<matype_id>
        initial_ma_styles; // Martial art styles that can be chosen upon character generation
    private:
        translation raw_name;
        translation raw_desc;
    public:
        std::string name() const;
        std::string desc() const;

        /**
         * Returns the color to display the mutation name with.
         */
        nc_color get_display_color() const;
        /**
         * Returns true if a character with this mutation shouldn't be able to wear given item.
         */
        bool conflicts_with_item( const item &it ) const;
        /**
         * Returns damage resistance on a given body part granted by this mutation.
         */
        const resistances &damage_resistance( body_part bp ) const;
        /**
         * Shortcut for getting the name of a (translated) mutation, same as
         * @code get( mutation_id ).name @endcode
         */
        static std::string get_name( const trait_id &mutation_id );
        /**
         * All known mutations. Key is the mutation id, value is the mutation_branch that you would
         * also get by calling @ref get.
         */
        static const std::vector<mutation_branch> &get_all();
        // For init.cpp: reset (clear) the mutation data
        static void reset_all();
        // For init.cpp: load mutation data from json
        void load( JsonObject &jo, const std::string &src );
        static void load_trait( JsonObject &jo, const std::string &src );
        // For init.cpp: check internal consistency (valid ids etc.) of all mutations
        static void check_consistency();

        /**
         * Load a trait blacklist specified by the given JSON object.
         */
        static void load_trait_blacklist( JsonObject &jsobj );

        /**
         * Check if the trait with the given ID is blacklisted.
         */
        static bool trait_is_blacklisted( const trait_id &tid );

        /** called after all JSON has been read and performs any necessary cleanup tasks */
        static void finalize();
        static void finalize_trait_blacklist();

        /**
         * @name Trait groups
         *
         * Trait groups are used to generate a randomized set of traits.
         * You usually only need the @ref Trait_group::traits_from function to
         * create traits from a group.
         */
        /*@{*/
        /**
         * Callback for the init system (@ref DynamicDataLoader), loads a trait
         * group definitions.
         * @param jsobj The json object to load from.
         * @throw std::string if the json object contains invalid data.
         */
        static void load_trait_group( JsonObject &jsobj );

        /**
         * Load a trait group from json. It differs from the other load_trait_group function as it
         * uses the group ID and subtype given as parameters, and does not look them up in
         * the json data (i.e. the given json object does not need to have them).
         *
         * This is intended for inline definitions of trait groups, e.g. in NPC class definitions:
         * the trait group there is embedded into the class type definition.
         *
         * @param jsobj The json object to load from.
         * @param gid The ID of the group that is to be loaded.
         * @param subtype The type of the trait group, either "collection", "distribution" or "old"
         * (i.e. the old list-based format, `[ ["TRAIT", 100] ]`).
         * @throw std::string if the json object contains invalid data.
         */
        static void load_trait_group( JsonObject &jsobj, const trait_group::Trait_group_tag &gid,
                                      const std::string &subtype );

        /**
         * Like the above function, except this function assumes that the given
         * array is the "entries" member of the trait group.
         *
         * For each element in the array, @ref mutation_branch::add_entry is called.
         *
         * Assuming the input array looks like `[ x, y, z ]`, this function loads it like the
         * above would load this object:
         * \code
         * {
         *      "subtype": "depends on is_collection parameter",
         *      "id": "ident",
         *      "entries": [ x, y, z ]
         * }
         * \endcode
         * Note that each entry in the array has to be a JSON object. The other function above
         * can also load data from arrays of strings, where the strings are item or group ids.
         */
        static void load_trait_group( JsonArray &entries, const trait_group::Trait_group_tag &gid,
                                      bool is_collection );

        /**
         * Create a new trait group as specified by the given JSON object and register
         * it as part of the given trait group.
         */
        static void add_entry( Trait_group &tg, JsonObject &obj );

        /**
         * Get the trait group object specified by the given ID, or null if no
         * such group exists.
         */
        static std::shared_ptr<Trait_group> get_group( const trait_group::Trait_group_tag &gid );

        /**
         * Return the idents of all trait groups that are known.
         */
        static std::vector<trait_group::Trait_group_tag> get_all_group_names();
};

struct mutation_category_trait {
    private:
        std::string raw_name;
        // Message when you consume mutagen
        std::string raw_mutagen_message;
        // Message when you inject an iv
        std::string raw_iv_message;
        std::string raw_iv_sound_message = "NULL";
        std::string raw_iv_sound_id = "shout";
        std::string raw_iv_sound_variant = "default";
        std::string raw_iv_sleep_message = "NULL";
        std::string raw_junkie_message;
        // Memorial message when you cross a threshold
        std::string raw_memorial_message;

    public:
        std::string name() const;
        std::string mutagen_message() const;
        std::string iv_message() const;
        std::string iv_sound_message() const;
        std::string iv_sound_id() const;
        std::string iv_sound_variant() const;
        std::string iv_sleep_message() const;
        std::string junkie_message() const;
        std::string memorial_message_male() const;
        std::string memorial_message_female() const;

        // Mutation category i.e "BIRD", "CHIMERA"
        std::string id;
        // The trait that you gain when you break the threshold for this category
        trait_id threshold_mut;

        // These are defaults
        int mutagen_hunger  = 10;
        int mutagen_thirst  = 10;
        int mutagen_pain    = 2;
        int mutagen_fatigue = 5;
        int mutagen_morale  = 0;
        // The minimum mutations an injection provides
        int iv_min_mutations    = 1;
        int iv_additional_mutations = 2;
        // Chance of additional mutations
        int iv_additional_mutations_chance = 3;
        int iv_hunger   = 10;
        int iv_thirst   = 10;
        int iv_pain     = 2;
        int iv_fatigue  = 5;
        int iv_morale   = 0;
        int iv_morale_max = 0;
        // Determines if you make a sound when you inject mutagen
        bool iv_sound = false;
        // The amount of noise produced by the sound
        int iv_noise = 0;
        // Whether the iv has a chance of knocking you out.
        bool iv_sleep = false;
        int iv_sleep_dur = 0;

        static const std::map<std::string, mutation_category_trait> &get_all();
        static const mutation_category_trait &get_category( const std::string &category_id );
        static void reset();
        static void check_consistency();

        static void load( JsonObject &jsobj );
};

void load_mutation_type( JsonObject &jsobj );
bool mutation_category_is_valid( const std::string &cat );
bool mutation_type_exists( const std::string &id );
std::vector<trait_id> get_mutations_in_types( const std::set<std::string> &ids );
std::vector<trait_id> get_mutations_in_type( const std::string &id );
bool trait_display_sort( const trait_id &a, const trait_id &b ) noexcept;

enum class mutagen_technique : int {
    consumed_mutagen,
    injected_mutagen,
    consumed_purifier,
    injected_purifier,
    injected_smart_purifier,
    num_mutagen_techniques // last
};

template<>
struct enum_traits<mutagen_technique> {
    static constexpr mutagen_technique last = mutagen_technique::num_mutagen_techniques;
};

enum class mutagen_rejection {
    accepted,
    rejected,
    destroyed
};

struct mutagen_attempt {
    mutagen_attempt( bool a, int c ) : allowed( a ), charges_used( c ) {}
    bool allowed;
    int charges_used;
};

mutagen_attempt mutagen_common_checks( player &p, const item &it, bool strong,
                                       mutagen_technique technique );

void test_crossing_threshold( Character &p, const mutation_category_trait &m_category );

#endif
