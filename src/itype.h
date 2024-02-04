#pragma once
#ifndef CATA_SRC_ITYPE_H
#define CATA_SRC_ITYPE_H

#include <array>
#include <cstddef>
#include <iosfwd>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "bodypart.h"
#include "calendar.h"
#include "color.h" // nc_color
#include "damage.h"
#include "enums.h" // point
#include "explosion.h"
#include "game_constants.h"
#include "item_pocket.h"
#include "iuse.h" // use_function
#include "mapdata.h"
#include "proficiency.h"
#include "relic.h"
#include "stomach.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

class Item_factory;
class JsonObject;
class item;
struct tripoint;
template <typename E> struct enum_traits;

enum art_effect_active : int;
enum art_charge : int;
enum art_charge_req : int;
enum art_effect_passive : int;

class gun_modifier_data
{
    private:
        translation name_;
        int qty_;
        std::set<std::string> flags_;

    public:
        gun_modifier_data( const translation &n, const int q, const std::set<std::string> &f ) : name_( n ),
            qty_( q ), flags_( f ) { }
        const translation &name() const {
            return name_;
        }
        /// @returns The translated name of the gun mode.
        std::string tname() const {
            return name_.translated();
        }
        int qty() const {
            return qty_;
        }
        const std::set<std::string> &flags() const {
            return flags_;
        }
};

class gunmod_location
{
    private:
        std::string _id;

    public:
        gunmod_location() = default;
        explicit gunmod_location( const std::string &id ) : _id( id ) { }

        /// Returns the translated name.
        std::string name() const;
        /// Returns the location id.
        std::string str() const {
            return _id;
        }

        bool operator==( const gunmod_location &rhs ) const {
            return _id == rhs._id;
        }
        bool operator<( const gunmod_location &rhs ) const {
            return _id < rhs._id;
        }

        void deserialize( std::string &&id ) {
            _id = std::move( id );
        }
};

struct islot_tool {
    std::set<ammotype> ammo_id;

    translation revert_msg;

    itype_id subtype;

    int max_charges = 0;
    int def_charges = 0;
    int charge_factor = 1;
    int charges_per_use = 0;
    int turns_per_charge = 0;
    units::power power_draw = 0_W;

    float fuel_efficiency = -1.0f;

    std::vector<int> rand_charges;
};

constexpr float base_metabolic_rate =
    2500.0f;  // kcal / day, standard average for human male, but game does not differentiate genders here.

struct islot_comestible {
    public:
        friend Item_factory;
        friend item;
        /** subtype, e.g. FOOD, DRINK, MED */
        std::string comesttype;

        /** tool needed to consume (e.g. lighter for cigarettes) */
        itype_id tool = itype_id::NULL_ID();

        /** Defaults # of charges (drugs, loaf of bread? etc) */
        int def_charges = 0;

        /** effect on character thirst (may be negative) */
        int quench = 0;

        /** Nutrition values to use for this type when they aren't calculated from
         * components */
        nutrients default_nutrition;

        /** Time until becomes rotten at standard temperature, or zero if never spoils */
        time_duration spoils = 0_turns;

        /** list of addictions and their potential */
        std::map<addiction_id, int> addictions;

        /** stimulant effect */
        int stim = 0;

        /**fatigue altering effect*/
        int fatigue_mod = 0;

        /** Reference to other item that replaces this one as a component in recipe results */
        itype_id cooks_like;

        /** Reference to item that will be received after smoking current item */
        itype_id smoking_result;

        /** TODO: add documentation */
        int healthy = 0;

        /** chance (odds) of becoming parasitised when eating (zero if never occurs) */
        int parasites = 0;

        /** freezing point in degrees celsius, below this temperature item can freeze */
        float freeze_point = 0;

        /** pet food category */
        std::set<std::string> petfood;

        /**effect on conditions to apply on consumption*/
        std::vector<effect_on_condition_id> consumption_eocs;

        /**List of diseases carried by this comestible and their associated probability*/
        std::map<diseasetype_id, int> contamination;

        // Materials to generate the below
        std::map<material_id, int> materials;
        //** specific heats in J/(g K) and latent heat in J/g */
        float specific_heat_liquid = 4.186f;
        float specific_heat_solid = 2.108f;
        float latent_heat = 333.0f;

        /** A penalty applied to fun for every time this food has been eaten in the last 48 hours */
        int monotony_penalty = -1;

        /** 1 nutr ~= 8.7kcal (1 nutr/5min = 288 nutr/day at 2500kcal/day) */
        static constexpr float kcal_per_nutr = base_metabolic_rate / ( 12 * 24 );

        bool has_calories() const {
            return default_nutrition.calories > 0;
        }

        int get_default_nutr() const {
            return default_nutrition.kcal() / kcal_per_nutr;
        }

        /** The monster group that is drawn from when the item rots away */
        mongroup_id rot_spawn = mongroup_id::NULL_ID();

        /** Chance the above monster group spawns*/
        int rot_spawn_chance = 10;

    private:
        /** effect on morale when consuming */
        int fun = 0;

        /** addiction potential to use when an addiction was given without one */
        int default_addict_potential = 0;
};

struct islot_brewable {
    /** What are the results of fermenting this item? */
    std::map<itype_id, int> results;

    /** How long for this brew to ferment. */
    time_duration time = 0_turns;

    bool was_loaded = false;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo );
};

/** Material data for individual armor body parts */
struct part_material {
    material_id id; //material type
    int cover; //portion coverage % of this material
    float thickness; //portion thickness of this material
    bool ignore_sheet_thickness = false; //if the def should ignore thickness of materials sheets

    bool operator ==( const part_material &comp ) const {
        return id == comp.id && cover == comp.cover && thickness == comp.thickness;
    }
    part_material() : id( material_id::NULL_ID() ), cover( 100 ), thickness( 0.0f ) {}
    part_material( material_id id, int cover, float thickness ) :
        id( id ), cover( cover ), thickness( thickness ) {}
    part_material( const std::string &id, int cover, float thickness ) :
        id( material_id( id ) ), cover( cover ), thickness( thickness ) {}

    void deserialize( const JsonObject &jo );
};

// values for attributes related to encumbrance
enum class encumbrance_modifier : int {
    IMBALANCED = 0,
    RESTRICTS_NECK,
    WELL_SUPPORTED,
    NONE,
    last
};

template<>
struct enum_traits<encumbrance_modifier> {
    static constexpr encumbrance_modifier last = encumbrance_modifier::last;
};

// if it is a multiplier or flat modifier
enum class encumbrance_modifier_type : int {
    MULT = 0,
    FLAT,
    last
};

struct armor_portion_data {

    // The base volume for an item
    static constexpr units::volume volume_per_encumbrance = 250_ml; // NOLINT(cata-serialize)

    // descriptors used to infer encumbrance
    std::vector<encumbrance_modifier> encumber_modifiers;

    // How much this piece encumbers the player.
    int encumber = 0;

    // When storage is full, how much it encumbers the player.
    int max_encumber = -1;

    // how much an item can hold comfortably compared to an average item
    float volume_encumber_modifier = 1;

    // Percentage of the body part that this item covers.
    // This determines how likely it is to hit the item instead of the player.
    int coverage = 0;
    int cover_melee = 0;
    int cover_ranged = 0;
    int cover_vitals = 0;

    /**
     * Average material thickness for all materials from
     * this armor portion
     */
    float avg_thickness = 0.0f;
    /**
     * Resistance to environmental effects.
     */
    int env_resist = 0;
    /**
     * Environmental protection of a gas mask with installed filter.
     */
    int env_resist_w_filter = 0;

    /**
     * What materials this portion is made of, for armor purposes.
     * Includes material portion and thickness.
     */
    std::vector<part_material> materials;

    // Where does this cover if any
    std::optional<body_part_set> covers;

    std::set<sub_bodypart_str_id> sub_coverage;

    // What layer does it cover if any
    std::set<layer_level> layers;

    // these are pre-calc values to save us time later

    // the chance that every material applies to an attack
    // this is primarily used as a cached value for UI
    int best_protection_chance = 100; // NOLINT(cata-serialize)

    // the chance that the smallest number of materials possible applies to an attack
    // this is primarily used as a cached value for UI
    int worst_protection_chance = 0; // NOLINT(cata-serialize)

    // this is to test if the armor has unique layering information
    bool has_unique_layering = false; // NOLINT(cata-serialize)

    // how breathable this part of the armor is
    // cached from the material data
    // only tracked for amalgamized body parts entries
    // if left the default -1 the value will be recalculated,
    int breathability = -1; // NOLINT(cata-serialize)

    // if this item is rigid, can't be worn with other rigid items
    bool rigid = false; // NOLINT(cata-serialize)

    // if this item only conflicts with rigid items that share a direct layer with it
    bool rigid_layer_only = false;

    // if this item is comfortable to wear without other items bellow it
    bool comfortable = false; // NOLINT(cata-serialize)

    /**
     * Returns the amount all sublocations this item covers could possibly
     * cover the specific body part.
     * This is used for converting from sub location coverage values
     * to body part coverage values. EX: shin guards cover the whole shin 100%
     * coverage. However only cover 35% of the overall leg.
     */
    int max_coverage( bodypart_str_id bp ) const;

    // checks if two entries are similar enough to be consolidated
    static bool should_consolidate( const armor_portion_data &l, const armor_portion_data &r );

    // helper function to return encumbrance value by descriptor and weight
    int calc_encumbrance( units::mass weight, bodypart_id bp ) const;

    // converts a specific encumbrance modifier to an actual encumbrance value
    static std::tuple<encumbrance_modifier_type, int> convert_descriptor_to_val(
        encumbrance_modifier em );

    void deserialize( const JsonObject &jo );
};

struct islot_armor {
    public:

        // thresholds for an item to count as hard / comfortable to wear
        static const int test_threshold = 40;

        /**
        * Whether this item can be worn on either side of the body
        */
        bool sided = false;
        /**
         * The Non-Functional variant of this item. Currently only applies to ablative plates
         */
        itype_id non_functional;
        /**
         * The verb for what happens when the item transforms to non-functional
         */
        translation damage_verb;
        /**
         * How much warmth this item provides.
         */
        int warmth = 0;
        /**
        * Factor modifying weight capacity
        */
        float weight_capacity_modifier = 1.0f;
        /**
        * Bonus to weight capacity
        */
        units::mass weight_capacity_bonus = 0_gram;
        /**
         * Whether this is a power armor item.
         */
        bool power_armor = false;
        /**
         * Whether this item has ablative pockets
         */
        bool ablative = false;
        /**
         * Whether this item has pockets that generate additional encumbrance
         */
        bool additional_pocket_enc = false;
        /**
         * Whether this item has pockets that can be ripped off
         */
        bool ripoff_chance = false;

        /**
         * If the entire item is rigid
         */
        bool rigid = false;

        /**
         * If the entire item is comfortable
         */
        bool comfortable = true;

        /**
         * Whether this item has pockets that are noisy
         */
        bool noisy = false;
        /**
         * Whitelisted clothing mods.
         * Restricted clothing mods must be listed here by id to be compatible.
         */
        std::vector<std::string> valid_mods;

        /**
         * If the item in question has any sub coverage when testing for encumbrance
         */
        bool has_sub_coverage = false;

        // Layer, encumbrance and coverage information for each body part.
        // This isn't directly loaded in but is instead generated from the loaded in
        // sub_data vector
        std::vector<armor_portion_data> data;

        // Layer, encumbrance and coverage information for each sub body part.
        // This vector can have duplicates for body parts themselves.
        std::vector<armor_portion_data> sub_data;

        // all of the layers this item is involved in
        std::vector<layer_level> all_layers;

        bool was_loaded = false;

        int avg_env_resist() const;
        int avg_env_resist_w_filter() const;
        float avg_thickness() const;

        void load( const JsonObject &jo );
        void deserialize( const JsonObject &jo );

    private:
        // Base material thickness, used to derive thickness in sub_data
        std::optional<float> _material_thickness = 0.0f;
        std::optional<int> _env_resist = 0;
        std::optional<int> _env_resist_w_filter = 0;
};

struct islot_pet_armor {
    /**
     * TODO: document me.
     */
    float thickness = 0;
    /**
     * Resistance to environmental effects.
     */
    int env_resist = 0;
    /**
     * Environmental protection of a gas mask with installed filter.
     */
    int env_resist_w_filter = 0;
    /**
     * The maximum volume a pet can be and wear this armor
     */
    units::volume max_vol = 0_ml;
    /**
     * The minimum volume a pet can be and wear this armor
     */
    units::volume min_vol = 0_ml;
    /**
     * What animal bodytype can wear this armor
     */
    std::string bodytype = "none";
    /**
     * Whether this is a power armor item.
     */
    bool power_armor = false;

    bool was_loaded = false;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo );
};

struct islot_book {
    /**
     * Which skill it upgrades, if any. Can be @ref skill_id::NULL_ID.
     */
    skill_id skill = skill_id::NULL_ID();
    /**
     * Which martial art it teaches.  Can be @ref matype_id::NULL_ID.
     */
    matype_id martial_art = matype_id::NULL_ID();
    /**
     * The skill level the book provides.
     */
    int level = 0;
    /**
     * The skill level required to understand it.
     */
    int req = 0;
    /**
     * How fun reading this is, can be negative.
     */
    int fun = 0;
    /**
     * Intelligence required to read it.
     */
    int intel = 0;
    /**
     * How long it takes to read.
     * "To read" means getting 1 skill point, not all of them.
     */
    time_duration time = 0_turns;
    /**
     * Fun books have chapters; after all are read, the book is less fun.
     */
    int chapters = 0;
    /**
     * What recipes can be learned from this book.
     */
    struct recipe_with_description_t {
        /**
         * The recipe that can be learned (never null).
         */
        const class recipe *recipe;
        /**
         * The skill level required to learn the recipe.
         */
        int skill_level;
        /**
         * The name for the recipe as it appears in the book.
         */
        std::optional<translation> optional_name;
        /**
         * Hidden means it does not show up in the description of the book.
         */
        bool hidden;
        bool operator<( const recipe_with_description_t &rhs ) const {
            return recipe < rhs.recipe;
        }
        bool is_hidden() const {
            return hidden;
        }
        std::string name() const;
    };
    using recipe_list_t = std::set<recipe_with_description_t>;
    recipe_list_t recipes;
    std::vector<book_proficiency_bonus> proficiencies;

    bool was_loaded = false;
    bool is_scannable = false;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo );
};

struct islot_mod {
    /** If non-empty restrict mod to items with those base (before modifiers) ammo types */
    std::set<ammotype> acceptable_ammo;

    /** If set modifies parent ammo to this type */
    std::set<ammotype> ammo_modifier;

    /** If non-empty replaces the compatible magazines for the parent item */
    std::map< ammotype, std::set<itype_id> > magazine_adaptor;

    /**
     * Pockets the mod will add to the item.
     * Any MAGAZINE_WELL or MAGAZINE type pockets will be overwritten,
     * and CONTAINER pockets will be added.
     */
    std::vector<pocket_data> add_pockets;

    /** Proportional adjustment of parent item ammo capacity */
    float capacity_multiplier = 1.0f;
};

/**
 * Common data for ranged things: guns, gunmods and ammo.
 * The values of the gun itself, its mods and its current ammo (optional) are usually summed
 * up in the item class and the sum is used.
 */
struct common_ranged_data {
    /**
     * Damage, armor piercing and multipliers for each.
     * If multipliers are set on both gun and ammo, values will be normalized
     * as in @ref damage_instance::add_damage
     */
    damage_instance damage;
    /**
     * Range bonus from gun.
     */
    int range = 0;
    /**
     * Range multiplier from gunmods or ammo.
     */
    float range_multiplier = 1.0;
    /**
     * Dispersion "bonus" from gun.
     */
    int dispersion = 0;
};

struct islot_engine {
        friend Item_factory;
        friend item;

    public:
        /** for combustion engines the displacement (cc) */
        int displacement = 0;

        bool was_loaded = false;

        void load( const JsonObject &jo );
        void deserialize( const JsonObject &jo );
};

struct islot_wheel {
    public:
        /** diameter of wheel (inches) */
        int diameter = 0;

        /** width of wheel (inches) */
        int width = 0;

        bool was_loaded = false;

        void load( const JsonObject &jo );
        void deserialize( const JsonObject &jo );
};

enum class itype_variant_kind : int {
    gun,
    generic,
    drug,
    last
};

template<>
struct enum_traits<itype_variant_kind> {
    static constexpr itype_variant_kind last = itype_variant_kind::last;
};

struct itype_variant_data {
    std::string id;
    translation alt_name;
    translation alt_description;
    ascii_art_id art;
    std::optional<std::string> alt_sym;
    std::optional<nc_color> alt_color = std::nullopt;

    bool append = false; // if the description should be appended to the base description.
    // Expand the description when generated and save it on the item
    bool expand_snippets = false;

    int weight = 1;

    void deserialize( const JsonObject &jo );
    void load( const JsonObject &jo );
};

// TODO: this shares a lot with the ammo item type, merge into a separate slot type?
struct islot_gun : common_ranged_data {
    /**
     * What skill this gun uses.
     */
    skill_id skill_used = skill_id::NULL_ID();
    /**
     * What type of ammo this gun uses.
     */
    std::set<ammotype> ammo;
    /**
     * Gun durability, affects gun being damaged during shooting.
     */
    int durability = 0;
    /**
     * For guns with an integral magazine what is the capacity?
     */
    int clip = 0;
    /**
     * Reload time, in moves.
     */
    int reload_time = 100;
    /**
     * Noise displayed when reloading the weapon.
     */
    translation reload_noise = to_translation( "click." );
    /**
     * Volume of the noise made when reloading this weapon.
     */
    int reload_noise_volume = 0;

    /** Maximum aim achievable using base weapon sights */
    int sight_dispersion = 30;

    /** Modifies base loudness as provided by the currently loaded ammo */
    int loudness = 0;

    /**
     * If this uses electric energy, how much (per shot).
     */
    units::energy energy_drain = 0_kJ;
    /**
     * One in X chance for gun to require major cleanup after firing blackpowder shot.
     */
    int blackpowder_tolerance = 8;
    /**
     * Minimum ammo recoil for gun to be able to fire more than once per attack.
     */
    int min_cycle_recoil = 0;
    /**
     * Length of gun barrel, if positive allows sawing down of the barrel
     */
    units::volume barrel_volume = 0_ml;
    units::length barrel_length = 0_mm;
    /**
     * Effects that are applied to the ammo when fired.
     */
    std::set<std::string> ammo_effects;
    /**
     * Location for gun mods.
     * Key is the location (untranslated!), value is the number of mods
     * that the location can have. The value should be > 0.
     */
    std::map<gunmod_location, int> valid_mod_locations;
    /**
    *Built in mods. string is id of mod. These mods will get the IRREMOVABLE flag set.
    */
    std::set<itype_id> built_in_mods;
    /**
    *Default mods, string is id of mod. These mods are removable but are default on the weapon.
    */
    std::set<itype_id> default_mods;

    /** Firing modes are supported by the gun. Always contains at least DEFAULT mode */
    std::map<gun_mode_id, gun_modifier_data> modes;

    /** How easy is control of recoil? If unset value automatically derived from weapon type */
    int handling = -1;

    /**
     *  Additional recoil applied per shot before effects of handling are considered
     *  @note useful for adding recoil effect to guns which otherwise consume no ammo
     */
    int recoil = 0;

    int ammo_to_fire = 1;

    /**
    * The amount by which the item's overheat value is reduced every turn. Used in
    * overheat-based guns.
    */
    double cooling_value = 100.0;

    /**
    *  Used only in overheat-based guns. No melting LMG barrels yet.
    */
    double heat_per_shot = 0.0;

    /**
    * Used in overheat-based guns.
    * Heat value at which critical overheat faults might occur.
    * A value beneath 0.0 means that the gun cannot overheat.
    */
    double overheat_threshold = -1.0;

    std::map<ammotype, std::set<itype_id>> cached_ammos;

    /**
     * Used for the skullgun cbm. Hurts the bodypart by that much when fired
     */
    std::map<bodypart_str_id, int> hurt_part_when_fired;
};

/// The type of gun. The second "_type" suffix is only to distinguish it from `item::gun_type`.
class gun_type_type
{
    private:
        std::string name_;

    public:
        /// @param name The untranslated name of the gun type. Must have been extracted
        /// for translation with the context "gun_type_type".
        explicit gun_type_type( const std::string &name ) : name_( name ) {}
        /// Translated name.
        std::string name() const;

        friend bool operator==( const gun_type_type &l, const gun_type_type &r ) {
            return l.name_ == r.name_;
        }

        friend struct std::hash<gun_type_type>;
};

namespace std
{

template<>
struct hash<gun_type_type> {
    size_t operator()( const gun_type_type &t ) const noexcept {
        return hash<std::string>()( t.name_ );
    }
};

} // namespace std

struct islot_gunmod : common_ranged_data {
    /** Where is this gunmod installed (e.g. "stock", "rail")? */
    gunmod_location location;

    /** What kind of weapons can this gunmod be used with (e.g. "rifle", "crossbow")? */
    std::unordered_set<gun_type_type> usable;

    /** If this value is set (non-negative), this gunmod functions as a sight. A sight is only usable to aim by a character whose current @ref Character::recoil is at or below this value. */
    int sight_dispersion = -1;

    /**
    * If the target has not appeared in the scope, the aiming speed is relatively low.
    * When the target appears in the scope, the aiming speed will be greatly accelerated.
    * FoV uses a more realistic method to reflect the aiming speed of the sight to insteadthe original abstract aim_speed
    */
    int field_of_view = -1;

    /**
     *  Its position has been replaced by FoV.
     *  But it is still retained due to compatibility considerations.
     */
    int aim_speed = -1;

    /**
    * This value is used to reflect other factors affecting aiming speed except Fov
    */
    double aim_speed_modifier = 0;

    /** Modifies base loudness as provided by the currently loaded ammo */
    int loudness = 0;

    /** How many moves does this gunmod take to install? */
    int install_time = -1;

    /** Increases base gun energy consumption by this many times per shot */
    float energy_drain_multiplier = 1.0f;

    /** Increases base gun energy consumption by this value per shot */
    units::energy energy_drain_modifier = 0_kJ;

    /** Increases base gun ammo to fire by this many times per shot */
    float ammo_to_fire_multiplier = 1.0f;

    /** Increases base gun ammo to fire by this value per shot */
    int ammo_to_fire_modifier = 0;

    /** Increases gun weight by this many times */
    float weight_multiplier = 1.0f;

    /** Firing modes added to or replacing those of the base gun */
    std::map<gun_mode_id, gun_modifier_data> mode_modifier;

    std::set<std::string> ammo_effects;

    /** Relative adjustment to base gun handling */
    int handling = 0;

    /** Percentage value change to the gun's loading time. Higher is slower */
    int reload_modifier = 0;

    /** Percentage value change to the gun's loading time. Higher is less likely */
    int consume_chance = 10000;

    /** Divsor to scale back gunmod consumption damage. lower is more damaging. Affected by ammo loudness and recoil, see ranged.cpp for how much. */
    int consume_divisor = 1;

    /** Enlarge or reduce shot spread */
    float shot_spread_multiplier_modifier = 0.0f;

    /** Modifies base strength required */
    int min_str_required_mod = 0;

    /** Additional gunmod slots to add to the gun */
    std::map<gunmod_location, int> add_mod;

    // wheter the item is supposed to work as a bayonet when attached
    bool is_bayonet = false;

    /** Not compatible on weapons that have this mod slot */
    std::set<gunmod_location> blacklist_slot;

    /** Not compatible on weapons that have these mods */
    std::set<itype_id> blacklist_mod;
    // hard coded barrel length from this mod
    units::length barrel_length = 0_mm;

    // minimum recoil to cycle while this is installed
    int overwrite_min_cycle_recoil = -1;

    //Manipulate overheat thresholds with fixed values and percentages
    double overheat_threshold_modifier = 0;
    float overheat_threshold_multiplier = 1.0f;

    //Manipulate cooling capacity with fixed values and percentages
    double cooling_value_modifier = 0;
    float cooling_value_multiplier = 1.0f;

    //Manipulation of generated heat by fixed values and percentages
    double heat_per_shot_modifier = 0;
    float heat_per_shot_multiplier = 1.0f;
};

struct islot_magazine {
    /** What type of ammo this magazine can be loaded with */
    std::set<ammotype> type;

    /** Capacity of magazine (in equivalent units to ammo charges) */
    int capacity = 0;

    /** Default amount of ammo contained by a magazine (often set for ammo belts) */
    int count = 0;

    /** Default type of ammo contained by a magazine (often set for ammo belts) */
    itype_id default_ammo = itype_id::NULL_ID();

    /** How long it takes to load each unit of ammo into the magazine */
    int reload_time = 100;

    /** For ammo belts one linkage (of given type) is dropped for each unit of ammo consumed */
    std::optional<itype_id> linkage;

    std::map<ammotype, std::set<itype_id>> cached_ammos;
    /** Map of [magazine type id] -> [set of gun itype_ids that accept the mag type ] */
    static std::map<itype_id, std::set<itype_id>> compatible_guns;
};

struct islot_battery {
    /** Maximum energy the battery can store */
    units::energy max_capacity;

    bool was_loaded = false;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo );
};

struct islot_ammo : common_ranged_data {
    /**
     * Ammo type, basically the "form" of the ammo that fits into the gun/tool.
     */
    ammotype type;
    /**
     * Type id of casings, if any.
     */
    std::optional<itype_id> casing;

    /**
     * Control chance for and state of any items dropped at ranged target
     *@{*/
    itype_id drop = itype_id::NULL_ID();

    float drop_chance = 1.0f;

    bool drop_active = true;
    /*@}*/

    /**
     * Default charges.
     */
    int def_charges = 1;

    /**
     * Number of projectiles fired per round, e.g. shotgun shot.
     */
    int count = 1;
    /**
     * Spread/dispersion between projectiles fired from the same round.
     */
    int shot_spread = 0;
    /**
     * Damage for a single shot.
     */
    damage_instance shot_damage;

    /**
     * TODO: document me.
     */
    std::set<std::string> ammo_effects;
    /**
     * Base loudness of ammo (possibly modified by gun/gunmods). If unspecified an
     * appropriate value is calculated based upon the other properties of the ammo
     */
    int loudness = -1;

    /** Recoil (per shot), roughly equivalent to kinetic energy (in Joules) */
    int recoil = 0;

    /**
     * Should this ammo explode in fire?
     * This value is cached by item_factory based on ammo_effects and item material.
     * @warning It is not read from the json directly.
     */
    bool cookoff = false;

    /**
     * Should this ammo apply a special explosion effect when in fire?
     * This value is cached by item_factory based on ammo_effects and item material.
     * @warning It is not read from the json directly.
     * */
    bool special_cookoff = false;

    /**
     * The damage multiplier to apply after a critical hit.
     */
    float critical_multiplier = 2.0f;

    /**
     * Some combat ammo might not have a damage value
     * Set this to make it show as combat ammo anyway
     */
    bool force_stat_display;

    bool was_loaded = false;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo );
};

struct islot_bionic {
    /**
     * Arbitrary difficulty scale, see bionics.cpp for its usage.
     */
    int difficulty = 0;
    /**
     * Id of the bionic, see bionics.cpp for its usage.
     */
    bionic_id id;
    /**
     * Whether this CBM is an upgrade of another.
     */
    bool is_upgrade = false;

    /**
    * Item with installation data that can be used to provide almost guaranteed successful install of corresponding bionic.
    */
    itype_id installation_data;
};

struct islot_seed {
    // Generic factory stuff
    bool was_loaded = false;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo );

    /**
     * Time it takes for a seed to grow (based of off a season length of 91 days).
     */
    time_duration grow = 0_turns;
    /**
     * Amount of harvested charges of fruits is divided by this number.
     */
    int fruit_div = 1;
    /**
     * Name of the plant.
     */
    translation plant_name;
    /**
     * What the plant sprouts into. Defaults to f_plant_seedling.
     */
    furn_str_id seedling_form;
    /**
     * What the plant grows into. Defaults to f_plant_mature.
     */
    furn_str_id mature_form;
    /**
     * The plant's final growth stage. Defaults to f_plant_harvest.
     */
    furn_str_id harvestable_form;
    /**
     * Type id of the fruit item.
     */
    itype_id fruit_id;
    /**
     * Whether to spawn seed items additionally to the fruit items.
     */
    bool spawn_seeds = true;
    /**
     * Additionally items (a list of their item ids) that will spawn when harvesting the plant.
     */
    std::vector<itype_id> byproducts;
    /**
     * Terrain tag required to plant the seed.
     */
    ter_furn_flag required_terrain_flag = ter_furn_flag::TFLAG_PLANTABLE;
    islot_seed() = default;
};

enum condition_type {
    FLAG,
    COMPONENT_ID,
    COMPONENT_ID_SUBSTRING,
    VAR,
    SNIPPET_ID,
    num_condition_types
};

template<>
struct enum_traits<condition_type> {
    static constexpr condition_type last = condition_type::num_condition_types;
};

// A name that is applied under certain conditions.
struct conditional_name {
    // Context type  (i.e. "FLAG"          or "COMPONENT_ID")
    condition_type type;
    // Context name  (i.e. "CANNIBALISM"   or "mutant")
    std::string condition;
    // Used for variables and snippets to identify the specific value
    std::string value;
    // Name to apply (i.e. "Luigi lasagne" or "smoked mutant"). Can use %s which will
    // be replaced by the item's normal name and/or preceding conditional names.
    translation name;
};

class islot_milling
{
    public:
        itype_id into_;
        double conversion_rate_ = 0;

        bool was_loaded = false;

        void load( const JsonObject &jo );
        void deserialize( const JsonObject &jo );
};

struct memory_card_info {
    float data_chance;
    itype_id on_read_convert_to;

    float photos_chance;
    int photos_amount;

    float songs_chance;
    int songs_amount;

    float recipes_chance;
    int recipes_amount;
    int recipes_level_min;
    int recipes_level_max;
    std::set<std::string> recipes_categories;
    bool secret_recipes;
};

struct itype {
        friend class Item_factory;
        friend struct mod_tracker;

        using FlagsSetType = std::set<flag_id>;

        /**
         * Slots for various item type properties. Each slot may contain a valid pointer or null, check
         * this before using it.
         */
        /*@{*/
        cata::value_ptr<islot_tool> tool;
        cata::value_ptr<islot_comestible> comestible;
        cata::value_ptr<islot_brewable> brewable;
        cata::value_ptr<islot_armor> armor;
        cata::value_ptr<islot_pet_armor> pet_armor;
        cata::value_ptr<islot_book> book;
        cata::value_ptr<islot_mod> mod;
        cata::value_ptr<islot_engine> engine;
        cata::value_ptr<islot_wheel> wheel;
        cata::value_ptr<islot_gun> gun;
        cata::value_ptr<islot_gunmod> gunmod;
        cata::value_ptr<islot_magazine> magazine;
        cata::value_ptr<islot_battery> battery;
        cata::value_ptr<islot_bionic> bionic;
        cata::value_ptr<islot_ammo> ammo;
        cata::value_ptr<islot_seed> seed;
        cata::value_ptr<relic> relic_data;
        cata::value_ptr<islot_milling> milling_data;
        /*@}*/

        /** Action to take BEFORE the item is placed on map. If it returns non-zero, item won't be placed. */
        use_function drop_action;

        /** Action to take when countdown expires */
        use_function countdown_action;

        /** Actions to take when item is processed */
        std::map<std::string, use_function> tick_action;

        /**
        * @name Non-negative properties
        * After loading from JSON these properties guaranteed to be zero or positive
        */
        /**@{*/

        /** Weight of item ( or each stack member ) */
        units::mass weight = 0_gram;
        /** Weight difference with the part it replaces for mods (defaults to weight) */
        units::mass integral_weight = -1_gram;

        std::vector<std::pair<itype_id, mod_id>> src;

        // Potential variant items that exist of this type (same stats, different name and desc)
        std::vector<itype_variant_data> variants;

        // A list of conditional names, in order of ascending priority.
        std::vector<conditional_name> conditional_names;

        /** Base damage output when thrown */
        damage_instance thrown_damage;

        /** What recipes can make this item */
        std::vector<recipe_id> recipes;

        // information related to being able to store things inside the item.
        std::vector<pocket_data> pockets;

        // What it has to say.
        std::vector<std::string> chat_topics;

        // a hint for tilesets: if it doesn't have a tile, what does it look like?
        itype_id looks_like;

        // Rather than use its own materials to determine repair difficulty, the item uses this item's materials
        itype_id repairs_like;

        std::string snippet_category;

        ascii_art_id picture_id;

        /** If set via JSON forces item category to this (preventing automatic assignment) */
        item_category_id category_force;

        std::string sym;

        requirement_id template_requirements;

    protected:
        itype_id id = itype_id::NULL_ID(); /** unique string identifier for this type */

    public:
        // The container it comes in
        std::optional<itype_id> default_container;
        std::optional<std::string> default_container_variant;

        std::set<weapon_category_id> weapon_category;

        // Tool qualities and levels for those that work even when tool is not charged
        std::map<quality_id, int> qualities;
        // Tool qualities that work only when the tool has charges_to_use charges remaining
        std::map<quality_id, int> charged_qualities;

        // True if this has given quality or charged_quality (regardless of current charge).
        bool has_any_quality( std::string_view quality ) const;

        // Properties are assigned to the type (belong to the item definition)
        std::map<std::string, std::string> properties;

        // Item vars are loaded from the type, but assigned and de/serialized with the item itself
        std::map<std::string, std::string> item_variables;

        // What we're made of (material names). .size() == made of nothing.
        // First -> the material
        // Second -> the portion of item covered by the material (portion / total portions)
        // MATERIALS WORK IN PROGRESS.
        std::map<material_id, int> materials;
        std::set<material_id> repairs_with;

        // This stores the first inserted material so that it can be used if all materials
        // are equivalent in proportion as a default
        // TODO: This is really legacy behavior and should maybe be removed
        material_id default_mat;

        /** Actions an instance can perform (if any) indexed by action type */
        std::map<std::string, use_function> use_methods;

        // @return returns itype_id of first ammo_id or itype_id::NULL_ID if not tool or no ammo defined
        const itype_id &tool_slot_first_ammo() const;

        /** The factor of ammo consumption indexed by action type*/
        std::map<std::string, int> ammo_scale;

        /** Fields to emit when item is in active state */
        std::set<emit_id> emits;

        std::set<matec_id> techniques;

        // Minimum stat(s) or skill(s) to use the item
        std::map<skill_id, int> min_skills;

        /** What items can be used to repair this item? @see Item_factory::finalize */
        std::set<itype_id> repair;

        /** What faults (if any) can occur */
        std::set<fault_id> faults;

        /** Magazine types (if any) for each ammo type that can be used to reload this item */
        std::map< ammotype, std::set<itype_id> > magazines;

        /** Default magazine for each ammo type that can be used to reload this item */
        std::map< ammotype, itype_id > magazine_default;

        // itemgroup used to generate the recipes within nanofabricator templates.
        item_group_id nanofab_template_group;

        // used for corpses placed by mapgen
        mtype_id source_monster = mtype_id::NULL_ID();
    private:
        FlagsSetType item_tags;

    public:
        // memory card related per-type static data
        cata::value_ptr<memory_card_info> memory_card_data;
        // How should the item explode
        explosion_data explosion;

        translation description; // Flavor text

    protected:
        // private because is should only be accessed through itype::nname!
        // nname() is used for display purposes
        translation name = no_translation( "none" );

    public:
        // Total of item's material portions (materials->second)
        int mat_portion_total = 0;

        int min_str = 0;
        int min_dex = 0;
        int min_int = 0;
        int min_per = 0;

        // Type of the variant - so people can turn off certain types of variants
        itype_variant_kind variant_kind = itype_variant_kind::last;

        phase_id phase = phase_id::SOLID; // e.g. solid, liquid, gas

        /** If positive starts countdown to countdown_action at item creation */
        time_duration countdown_interval = 0_seconds;

        /**
        * If set the item will revert to this after countdown. If not set the item is deleted.
        * Tools revert to this when they run out of charges
        */
        std::optional<itype_id> revert_to;

        /**
        * Space occupied by items of this type
        * CAUTION: value given is for a default-sized stack. Avoid using where @ref count_by_charges items may be encountered; see @ref item::volume instead.
        * To determine how many of an item can fit in a given space, use @ref charges_per_volume.
        */
        units::volume volume = 0_ml;

        /**
        * Space consumed when integrated as part of another item (defaults to volume)
        * CAUTION: value given is for a default-sized stack. Avoid using this. In general, see @ref item::volume instead.
        */
        units::volume integral_volume = -1_ml;

        /**
        * How long the longest side of this item is. If undefined, calculated from volume instead.
        */
        units::length longest_side = -1_mm;

        /**
        * length added when integrated as part of another item (defaults to 0)
        */
        units::length integral_longest_side = -1_mm;

        /** Number of items per above volume for @ref count_by_charges items */
        int stack_size = 0;

        /** Value before the Cataclysm. Price given is for a default-sized stack. */
        units::money price = 0_cent;

        /** Value after the Cataclysm, dependent upon practical usages. Price given is for a default-sized stack. */
        units::money price_post = -1_cent;

        int m_to_hit = 0;  // To-hit bonus for melee combat; -5 to 5 is reasonable

        unsigned light_emission = 0;   // Exactly the same as item_tags LIGHT_*, this is for lightmap.

        nc_color color = c_white; // Color on the map (color.h)

        /**
        * How much insulation this item provides, either as a container, or as
        * a vehicle base part.  Larger means more insulation, less than 1 but
        * greater than zero, transfers faster, cannot be less than zero.
        */
        float insulation_factor = 1.0f;

        /**
        * Efficiency of solar energy conversion for solarpacks.
        */
        float solar_efficiency = 0.0f;

    private:
        /** maximum amount of damage to a non- count_by_charges item */
        static constexpr int damage_max_ = 4000;
        int degrade_increments_ = 50;

    public:
        /** Damage output in melee for zero or more damage types */
        std::unordered_map<damage_type_id, float> melee;

        bool default_container_sealed = true;

        // Should the item explode when lit on fire
        bool explode_in_fire = false;

        // used for generic_factory for copy-from
        bool was_loaded = false;

        // Expand snippets in the description and save the description on the object
        bool expand_snippets = false;

    private:
        // load-only, for applying proportional melee values at load time
        std::unordered_map<damage_type_id, float> melee_proportional;

        // load-only, for applying relative melee values at load time
        std::unordered_map<damage_type_id, float> melee_relative;

        /** Can item be combined with other identical items? */
        bool stackable_ = false;

    public:
        static constexpr int damage_scale = 1000; /** Damage scale compared to the old float damage value */

        itype() {
            melee.clear();
        }

        int damage_max() const {
            return count_by_charges() ? 0 : damage_max_;
        }
        /** Number of degradation increments before the item is destroyed */
        int degrade_increments() const {
            return degrade_increments_;
        }

        /**
        * Quantizes item damage numbers into levels (for example for display).
        * item damage [    0 -    0 ] returns 0
        * item damage [    1 -  999 ] returns 1
        * item damage [ 1000 - 1999 ] returns 2
        * item damage [ 2000 - 2999 ] returns 3
        * item damage [ 3000 - 3999 ] returns 4
        * item damage [ 4000 - 4000 ] returns 5
        */
        int damage_level( int damage ) const;

        std::string get_item_type_string() const;

        // Returns the name of the item type in the correct language and with respect to its grammatical number,
        // based on quantity (example: item type “anvil”, nname(4) would return “anvils” (as in “4 anvils”).
        std::string nname( unsigned int quantity ) const;

        // Allow direct access to the type id for the few cases that need it.
        itype_id get_id() const {
            return id;
        }

        bool count_by_charges() const {
            return stackable_ || ammo || ( comestible && phase != phase_id::SOLID );
        }

        int charges_default() const;

        int charges_to_use() const;

        // for tools that sub another tool, but use a different ratio of charges
        int charge_factor() const {
            return tool ? tool->charge_factor : 1;
        }

        int maximum_charges() const {
            if( tool ) {
                return tool->max_charges;
            }
            return 1;
        }
        bool can_have_charges() const;

        /**
         * Number of (charges of) this type of item that fit into the given volume.
         * May return 0 if not even one charge fits into the volume.
         */
        int charges_per_volume( const units::volume &vol ) const;

        bool has_use() const;

        bool has_flag( const flag_id &flag ) const;

        // returns read-only set of all item tags/flags
        const FlagsSetType &get_flags() const;

        bool can_use( const std::string &iuse_name ) const;
        const use_function *get_use( const std::string &iuse_name ) const;

        // Here "invoke" means "actively use". "Tick" means "active item working"
        std::optional<int> invoke( Character *p, item &it,
                                   const tripoint &pos ) const; // Picks first method or returns 0
        std::optional<int> invoke( Character *p, item &it, const tripoint &pos,
                                   const std::string &iuse_name ) const;
        int tick( Character *p, item &it, const tripoint &pos ) const;

        virtual ~itype() = default;

        // returns true if it is one of the outcomes of cutting
        bool is_basic_component() const;
};

void load_charge_removal_blacklist( const JsonObject &jo, std::string_view src );
void load_temperature_removal_blacklist( const JsonObject &jo, std::string_view src );

#endif // CATA_SRC_ITYPE_H
