#pragma once
#ifndef CATA_SRC_MUTATION_H
#define CATA_SRC_MUTATION_H

#include <climits>
#include <iosfwd>
#include <map>
#include <new>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "calendar.h"
#include "character.h"
#include "damage.h"
#include "hash_utils.h"
#include "memory_fast.h"
#include "point.h"
#include "translations.h"
#include "type_id.h"
#include "value_ptr.h"

class JsonArray;
class JsonObject;
class Trait_group;
class item;
class nc_color;
struct dream;

enum game_message_type : int;

template <typename E> struct enum_traits;

extern std::vector<dream> dreams;
extern std::map<mutation_category_id, std::vector<trait_id> > mutations_category;

struct dream {
    private:
        std::vector<translation> raw_messages; // The messages that the dream will give

    public:
        std::vector<std::string> messages() const;

        mutation_category_id category; // The category that will trigger the dream
        int strength; // The category strength required for the dream

        dream() {
            strength = 0;
        }

        static void load( const JsonObject &jsobj );
};

struct mut_attack {
    /** Text printed when the attack is proced by you */
    translation attack_text_u;
    /** As above, but for npc */
    translation attack_text_npc;
    /** Need all of those to qualify for this attack */
    std::set<trait_id> required_mutations;
    /** Need none of those to qualify for this attack */
    std::set<trait_id> blocker_mutations;

    /** If not empty, this body part needs to be uncovered for the attack to proc */
    bodypart_str_id bp;

    /** Chance to proc is one_in( chance - dex - unarmed ) */
    int chance = 0;

    damage_instance base_damage;
    /** Multiplied by strength and added to the above */
    damage_instance strength_damage;

    /** Should be true when and only when this attack needs hardcoded handling */
    bool hardcoded_effect = false;
};

struct mut_transform {

    trait_id target;

    /** displayed if player sees transformation with %s replaced by mutation name */
    translation msg_transform;
    /** used to set the active property of the transformed @ref target */
    bool active = false;
    /** subtracted from @ref Creature::moves when transformation is successful */
    int moves = 0;
    // If true the transformation uses the "normal" mutation rules - canceling conflicting traits etc
    bool safe = false;
    mut_transform();
    bool load( const JsonObject &jsobj, std::string_view member );
};

struct mut_personality_score {

    /** bounds within which this trait is applied */
    int min_aggression = -10;
    int max_aggression = 10;
    int min_bravery = -10;
    int max_bravery = 10;
    int min_collector = -10;
    int max_collector = 10;
    int min_altruism = -10;
    int max_altruism = 10;
    mut_personality_score();
    bool load( const JsonObject &jsobj, std::string_view member );
};

struct reflex_activation_data {

    /**What variable controls the activation*/
    std::function<bool( dialogue & )>trigger;

    std::pair<translation, game_message_type> msg_on;
    std::pair<translation, game_message_type> msg_off;

    bool is_trigger_true( Character &guy ) const;

    bool was_loaded = false;
    void load( const JsonObject &jsobj );
    void deserialize( const JsonObject &jo );
};

struct trait_and_var {
    trait_id trait;
    std::string variant;

    trait_and_var() = default;
    trait_and_var( const trait_id &t, const std::string &v ) : trait( t ), variant( v ) {}

    void deserialize( const JsonValue &jv );
    void serialize( JsonOut &jsout ) const;

    std::string name() const;
    std::string desc() const;

    bool operator==( const trait_and_var &other ) const {
        return trait == other.trait && variant == other.variant;
    }
};

struct trait_replacement {
    std::optional<proficiency_id> prof;
    std::optional<trait_and_var> trait;
    bool error = false;
};

struct mutation_variant {
    std::string id;

    translation alt_name;
    translation alt_description;

    // If the description should be appended to the non-variant description
    bool append_desc = false;

    // Used for automatic selection of variant when mutation is assigned
    // With 0 weight, the variant will never be automatically assigned
    int weight = 0;

    // !!THIS MUST BE SET AFTER LOADING!!
    // This is the trait that the variant corresponds to, so we can save/load correctly
    trait_id parent;

    void deserialize( const JsonObject &jo );
    void load( const JsonObject &jo );
};

struct mutation_branch {
        trait_id id;
        std::vector<std::pair<trait_id, mod_id>> src;
        bool was_loaded = false;
        // True if this is a valid mutation.
        bool valid = false;
        // True if Purifier can remove it (False for *Special* mutations).
        bool purifiable = false;
        // True if it's a threshold itself, and shouldn't be obtained *easily* (False by default).
        bool threshold = false;
        // Other threshold traits that are taken as acceptable replacements for this threshold
        std::vector<trait_id> threshold_substitutes;
        // Disallow threshold substitution for this trait in particular
        bool strict_threshreq = false;
        // True if this is a trait associated with professional training/experience, so profession/quest ONLY.
        bool profession = false;
        // True if the mutation is obtained through the debug menu
        bool debug = false;
        // True if the mutation should be displayed in the `@` menu
        bool player_display = true;
        // True if mutation is purely comestic and can be changed anytime without any effect
        bool vanity = false;
        // Dummy mutations are special; they're not gained through normal mutating, and will instead be targeted for the purposes of removing conflicting mutations
        bool dummy = false;
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
        // IF any of the four are true, it drains that as the "cost"
        bool sleepiness       = false;
        bool hunger        = false;
        bool thirst        = false;
        // How many points it costs in character creation
        int points     = 0;
        // How many mutagen vitamins are consumed to gain this trait
        int vitamin_cost = 100;
        int visibility = 0;
        int ugliness   = 0;
        int cost       = 0;
        // costs are consumed every cooldown turns,
        time_duration cooldown   = 0_turns;
        // bodytemp elements:
        units::temperature_delta bodytemp_min = 0_C_delta;
        units::temperature_delta bodytemp_max = 0_C_delta;
        // Additional bonuses
        std::optional<int> scent_intensity;

        int butchering_quality = 0;

        cata::value_ptr<mut_transform> transform;

        cata::value_ptr<mut_personality_score> personality_score;

        // Cosmetic variants of this mutation
        std::map<std::string, mutation_variant> variants;

        std::vector<std::vector<reflex_activation_data>> trigger_list;

        /**Map of crafting skills modifiers, can be negative*/
        std::map<skill_id, int> craft_skill_bonus;

        /**What do you smell like*/
        std::optional<scenttype_id> scent_typeid;

        /**Map of glowing body parts and their glow intensity*/
        std::map<bodypart_str_id, float> lumination;

        // Bonus or penalty to social checks (additive).  50 adds 50% to success, -25 subtracts 25%
        social_modifiers social_mods;

        /** The item, if any, spawned by the mutation */
        itype_id spawn_item;

        /**Species ignoring character with the mutation*/
        std::vector<species_id> ignored_by;

        /**Map of angered species and there intensity*/
        std::map<species_id, int> anger_relations;

        /**List of material required for food to be be edible*/
        std::set<material_id> can_only_eat;

        /**List of healing items allowed*/
        std::set<itype_id> can_only_heal_with;
        std::set<itype_id> can_heal_with;

        /**List of allowed mutation category*/
        std::set<mutation_category_id> allowed_category;

        /**List of body parts locked out of bionics*/
        std::set<bodypart_str_id> no_cbm_on_bp;

        // spells learned and their associated level when gaining the mutation
        std::map<spell_id, int> spells_learned;
        // hide activation menu when activating - preferred for spell targeting activations
        std::optional<bool> hide_on_activated = std::nullopt;
        // hide activation menu when deactivating - preferred for spell targeting deactivations
        std::optional<bool> hide_on_deactivated = std::nullopt;
        /** Monster cameras added by this mutation */
        std::map<mtype_id, int> moncams;
        /** effect_on_conditions triggered when this mutation activates */
        std::vector<effect_on_condition_id> activated_eocs;
        // if the above activated eocs should be run without turning on the mutation
        bool activated_is_setup = false;
        /** effect_on_conditions triggered while this mutation is active */
        std::vector<effect_on_condition_id> processed_eocs;
        /** effect_on_conditions triggered when this mutation deactivates */
        std::vector<effect_on_condition_id> deactivated_eocs;
        /** mutation enchantments */
        std::vector<enchantment_id> enchantments;

        struct OverrideLook {
            std::string id;
            std::string tile_category;
            OverrideLook( const std::string &_id, const std::string &_tile_category )
                : id( _id ), tile_category( _tile_category ) {}
        };
        /** ID, tile category, and variant
        This will make the player appear as another entity (such as a non-humanoid creature).
        The texture will override all other textures, including the character's body. */
        std::optional<OverrideLook> override_look;
    private:
        translation raw_spawn_item_message;
    public:
        std::string spawn_item_message() const;

        /** The fake gun, if any, spawned and fired by the ranged mutation */
        itype_id ranged_mutation;
    private:
        translation raw_ranged_mutation_message;
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
        std::vector<mutation_category_id> category; // Mutation Categories
        std::set<json_character_flag> flags; // Mutation flags
        std::set<json_character_flag> active_flags; // Mutation flags only when active
        std::set<json_character_flag> inactive_flags; // Mutation flags only when inactive
        std::map<bodypart_str_id, tripoint> protection; // Mutation wet effects
        std::map<bodypart_str_id, int> encumbrance_always; // Mutation encumbrance that always applies
        // Mutation encumbrance that applies when covered with unfitting item
        std::map<bodypart_str_id, int> encumbrance_covered;
        // a multiplier to encumbrance that is already modified by mutations
        std::map<bodypart_str_id, float> encumbrance_multiplier_always;
        // Body parts that now need OVERSIZE gear
        std::set<bodypart_str_id> restricts_gear;
        std::set<sub_bodypart_str_id> restricts_gear_subparts;
        // Body parts that will now already have rigid gear
        std::set<bodypart_str_id> remove_rigid;
        std::set<sub_bodypart_str_id> remove_rigid_subparts;
        // item flags that allow wearing gear even if its body part is restricted
        std::set<flag_id> allowed_items;
        // Mutation stat mods
        /** Key pair is <active: bool, mod type: "STR"> */
        std::unordered_map<std::pair<bool, std::string>, int, cata::tuple_hash> mods;
        std::map<bodypart_str_id, resistances> armor; // Modifiers to protection values
        std::vector<itype_id> integrated_armor; // Armor pseudo-items that are put on by this mutation
        std::vector<matype_id>
        initial_ma_styles; // Martial art styles that can be chosen upon character generation
    private:
        std::map<bodypart_str_id, int> bionic_slot_bonuses;
        translation raw_name;
        translation raw_desc;
    public:
        std::string name( const std::string &variant = "" ) const;
        std::string desc( const std::string &variant = "" ) const;

        /**
         * Returns the color to display the mutation name with.
         */
        nc_color get_display_color() const;
        /**
         * Picks a variant out of the possibilities based on their wieghts.
         * If all have zero weights, returns nullptr
         */
        const mutation_variant *pick_variant() const;
        /**
         * Returns a pointer to the mutation variant with corresponding id, or nullptr if none exists
         */
        const mutation_variant *variant( const std::string &id ) const;
        /**
         * Have the player pick a variant out of the options available.
         * Return a pointer to the variant
         * Returns nullptr and does not open a menu if there are no variants.
         */
        const mutation_variant *pick_variant_menu() const;
        /**
         * Returns true if a character with this mutation shouldn't be able to wear given item.
         */
        bool conflicts_with_item( const item &it ) const;
        /**
         * Returns true if a character with this mutation has to take off rigid items at the location.
         */
        bool conflicts_with_item_rigid( const item &it ) const;
        /**
         * Returns damage resistance on a given body part granted by this mutation.
         */
        const resistances &damage_resistance( const bodypart_id &bp ) const;
        /**
         * Returns bionic slot bonus on a given body part granted by this mutation
         */
        int bionic_slot_bonus( const bodypart_str_id &part ) const;
        /**
         * All known mutations. Key is the mutation id, value is the mutation_branch that you would
         * also get by calling @ref get.
         */
        static const std::vector<mutation_branch> &get_all();
        // For init.cpp: reset (clear) the mutation data
        static void reset_all();
        // For init.cpp: load mutation data from json
        void load( const JsonObject &jo, const std::string &src );
        static void load_trait( const JsonObject &jo, const std::string &src );
        // For init.cpp: check internal consistency (valid ids etc.) of all mutations
        static void check_consistency();

        /**
         * Load a trait blacklist specified by the given JSON object.
         */
        static void load_trait_blacklist( const JsonObject &jsobj );

        /**
         * Check if the trait with the given ID is blacklisted.
         */
        static bool trait_is_blacklisted( const trait_id &tid );

        /**
         * Load a trait migration given by JSON object.
         */
        static void load_trait_migration( const JsonObject &jo );

        /**
         * Give the appropriate replacement migrated trait
         */
        static const trait_replacement &trait_migration( const trait_id &tid );

        /** called after all JSON has been read and performs any necessary cleanup tasks */
        static void finalize_all();
        static void finalize_trait_blacklist();
        void finalize();

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
         * @throw JsonError if the json object contains invalid data.
         */
        static void load_trait_group( const JsonObject &jsobj );

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
         * @throw JsonError if the json object contains invalid data.
         */
        static void load_trait_group( const JsonObject &jsobj, const trait_group::Trait_group_tag &gid,
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
         *      "id": "identifier",
         *      "entries": [ x, y, z ]
         * }
         * \endcode
         * Note that each entry in the array has to be a JSON object. The other function above
         * can also load data from arrays of strings, where the strings are item or group ids.
         */
        static void load_trait_group( const JsonArray &entries, const trait_group::Trait_group_tag &gid,
                                      bool is_collection );

        /**
         * Create a new trait group as specified by the given JSON object and register
         * it as part of the given trait group.
         */
        static void add_entry( Trait_group &tg, const JsonObject &obj );

        /**
         * Get the trait group object specified by the given ID, or null if no
         * such group exists.
         */
        static shared_ptr_fast<Trait_group> get_group( const trait_group::Trait_group_tag &gid );

        /**
         * Return the idents of all trait groups that are known.
         */
        static std::vector<trait_group::Trait_group_tag> get_all_group_names();
};

struct mutation_category_trait {
    private:
        translation raw_name;
        // Message when you consume mutagen
        translation raw_mutagen_message;
        // Memorial message when you cross a threshold
        std::string raw_memorial_message;

    public:
        std::string name() const;
        std::string mutagen_message() const;
        std::string memorial_message_male() const;
        std::string memorial_message_female() const;

        // Meta-label indicating that the category isn't finished yet.
        bool wip = false;
        // Skip consistency tests for this category. This should only really be used on categories that only contain dummy mutations.
        bool skip_test = false;
        // Mutation category i.e "BIRD", "CHIMERA"
        mutation_category_id id;
        // The trait that you gain when you break the threshold for this category
        trait_id threshold_mut;
        // Amount of vitamin necessary to attempt breaking the threshold
        int threshold_min = 2200;
        // Mutation vitamin
        vitamin_id vitamin;
        // Chance to remove base traits
        int base_removal_chance = 100;
        // Multiplier of vitamin costs when mutating this category removes starting traits
        float base_removal_cost_mul = 3.0f;

        static const std::map<mutation_category_id, mutation_category_trait> &get_all();
        static const mutation_category_trait &get_category(
            const mutation_category_id &category_id );
        static void reset();
        static void check_consistency();

        static void load( const JsonObject &jsobj );
};

void load_mutation_type( const JsonObject &jsobj );
bool mutation_category_is_valid( const mutation_category_id &cat );
bool mutation_type_exists( const std::string &id );
std::vector<trait_id> get_mutations_in_types( const std::set<std::string> &ids );
std::vector<trait_id> get_mutations_in_type( const std::string &id );
bool mutation_is_in_category( const trait_id &mut, const mutation_category_id &cat );
std::vector<trait_and_var> mutations_var_in_type( const std::string &id );
bool trait_display_sort( const trait_and_var &a, const trait_and_var &b ) noexcept;
bool trait_display_nocolor_sort( const trait_and_var &a, const trait_and_var &b ) noexcept;

bool are_conflicting_traits( const trait_id &trait_a, const trait_id &trait_b );
bool b_is_lower_trait_of_a( const trait_id &trait_a, const trait_id &trait_b );
bool b_is_higher_trait_of_a( const trait_id &trait_a, const trait_id &trait_b );
bool are_opposite_traits( const trait_id &trait_a, const trait_id &trait_b );
bool are_same_type_traits( const trait_id &trait_a, const trait_id &trait_b );
bool contains_trait( std::vector<string_id<mutation_branch>> traits, const trait_id &trait );
int get_total_nonbad_in_category( const mutation_category_id &categ );

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

void test_crossing_threshold( Character &guy, const mutation_category_trait &m_category );

#endif // CATA_SRC_MUTATION_H
