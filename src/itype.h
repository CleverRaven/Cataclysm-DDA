#pragma once
#ifndef CATA_SRC_ITYPE_H
#define CATA_SRC_ITYPE_H

#include <array>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "bodypart.h" // body_part::num_bp
#include "calendar.h"
#include "color.h" // nc_color
#include "damage.h"
#include "enums.h" // point
#include "explosion.h"
#include "game_constants.h"
#include "iuse.h" // use_function
#include "optional.h"
#include "pldata.h" // add_type
#include "relic.h"
#include "stomach.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

class Item_factory;
class item;
class player;
struct tripoint;
template <typename E> struct enum_traits;

enum art_effect_active : int;
enum art_charge : int;
enum art_charge_req : int;
enum art_effect_passive : int;
using itype_id = std::string;

class gun_modifier_data
{
    private:
        std::string name_;
        int qty_;
        std::set<std::string> flags_;

    public:
        /**
         * @param n A string that can be translated via @ref _ (must have been extracted for translation).
         */
        gun_modifier_data( const std::string &n, const int q, const std::set<std::string> &f ) : name_( n ),
            qty_( q ), flags_( f ) { }
        std::string name() const {
            return name_;
        }
        /// @returns The translated name of the gun mode.
        std::string tname() const {
            return _( name_ );
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
        gunmod_location( const std::string &id ) : _id( id ) { }

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
};

struct islot_tool {
    std::set<ammotype> ammo_id;

    cata::optional<itype_id> revert_to;
    std::string revert_msg;

    std::string subtype;

    int max_charges = 0;
    int def_charges = 0;
    int charge_factor = 1;
    int charges_per_use = 0;
    int turns_per_charge = 0;
    int power_draw = 0;

    std::vector<int> rand_charges;
};

struct islot_comestible {
    public:
        friend Item_factory;
        friend item;
        /** subtype, e.g. FOOD, DRINK, MED */
        std::string comesttype;

        /** tool needed to consume (e.g. lighter for cigarettes) */
        std::string tool = "null";

        /** Defaults # of charges (drugs, loaf of bread? etc) */
        int def_charges = 1;

        /** effect on character thirst (may be negative) */
        int quench = 0;

        /** Nutrition values to use for this type when they aren't calculated from
         * components */
        nutrients default_nutrition;

        /** Time until becomes rotten at standard temperature, or zero if never spoils */
        time_duration spoils = 0_turns;

        /** addiction potential */
        int addict = 0;

        /** effects of addiction */
        add_type add = add_type::NONE;

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

        /**Amount of radiation you get from this comestible*/
        int radiation = 0;

        /** freezing point in degrees Fahrenheit, below this temperature item can freeze */
        int freeze_point = temperatures::freezing;

        /**List of diseases carried by this comestible and their associated probability*/
        std::map<diseasetype_id, int> contamination;

        //** specific heats in J/(g K) and latent heat in J/g */
        float specific_heat_liquid = 4.186;
        float specific_heat_solid = 2.108;
        float latent_heat = 333;

        /** A penalty applied to fun for every time this food has been eaten in the last 48 hours */
        int monotony_penalty = 2;

        /** 1 nutr ~= 8.7kcal (1 nutr/5min = 288 nutr/day at 2500kcal/day) */
        static constexpr float kcal_per_nutr = 2500.0f / ( 12 * 24 );

        bool has_calories() const {
            return default_nutrition.kcal > 0;
        }

        int get_default_nutr() const {
            return default_nutrition.kcal / kcal_per_nutr;
        }

        /** The monster group that is drawn from when the item rots away */
        mongroup_id rot_spawn = mongroup_id::NULL_ID();

        /** Chance the above monster group spawns*/
        int rot_spawn_chance = 10;

    private:
        /** effect on morale when consuming */
        int fun = 0;
};

struct islot_brewable {
    /** What are the results of fermenting this item? */
    std::vector<std::string> results;

    /** How long for this brew to ferment. */
    time_duration time = 0_turns;
};

struct islot_container {
    /**
     * Inner volume of the container.
     */
    units::volume contains = 0_ml;
    /**
     * Can be resealed.
     */
    bool seals = false;
    /**
     * Can hold liquids.
     */
    bool watertight = false;
    /**
     * Contents do not spoil.
     */
    bool preserves = false;
    /**
     * If this is set to anything but "null", changing this container's contents in any way
     * will turn this item into that type.
     */
    itype_id unseals_into = "null";
};

struct islot_armor {
    /**
     * Bitfield of enum body_part
     * TODO: document me.
     */
    body_part_set covers;
    /**
     * Whether this item can be worn on either side of the body
     */
    bool sided = false;
    /**
     * How much this item encumbers the player.
     */
    int encumber = 0;
    /**
    * When storage is full, how much it encumbers the player.
    */
    int max_encumber = 0;
    /**
     * Percentage of the body part area that this item covers.
     * This determines how likely it is to hit the item instead of the player.
     */
    int coverage = 0;
    /**
     * TODO: document me.
     */
    int thickness = 0;
    /**
     * Resistance to environmental effects.
     */
    int env_resist = 0;
    /**
     * Environmental protection of a gas mask with installed filter.
     */
    int env_resist_w_filter = 0;
    /**
     * How much warmth this item provides.
     */
    int warmth = 0;
    /**
     * How much storage this items provides when worn.
     */
    units::volume storage = 0_ml;
    /**
    * Factor modifiying weight capacity
    */
    float weight_capacity_modifier = 1.0;
    /**
    * Bonus to weight capacity
    */
    units::mass weight_capacity_bonus = 0_gram;
    /**
     * Whether this is a power armor item.
     */
    bool power_armor = false;
    /**
     * Whitelisted clothing mods.
     * Restricted clothing mods must be listed here by id to be compatible.
     */
    std::vector<std::string> valid_mods;
};

struct islot_pet_armor {
    /**
     * TODO: document me.
     */
    int thickness = 0;
    /**
     * Resistance to environmental effects.
     */
    int env_resist = 0;
    /**
     * Environmental protection of a gas mask with installed filter.
     */
    int env_resist_w_filter = 0;
    /**
     * How much storage this items provides when worn.
     */
    units::volume storage = 0_ml;
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
     * How long in minutes it takes to read.
     * "To read" means getting 1 skill point, not all of them.
     */
    int time = 0;
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
        std::string name;
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
    };
    using recipe_list_t = std::set<recipe_with_description_t>;
    recipe_list_t recipes;
};

struct islot_mod {
    /** If non-empty restrict mod to items with those base (before modifiers) ammo types */
    std::set<ammotype> acceptable_ammo;

    /** If set modifies parent ammo to this type */
    std::set<ammotype> ammo_modifier;

    /** If non-empty replaces the compatible magazines for the parent item */
    std::map< ammotype, std::set<itype_id> > magazine_adaptor;

    /** Proportional adjustment of parent item ammo capacity */
    float capacity_multiplier = 1.0;
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
};

struct islot_wheel {
    public:
        /** diameter of wheel (inches) */
        int diameter = 0;

        /** width of wheel (inches) */
        int width = 0;
};

struct fuel_explosion {
    int explosion_chance_hot = 0;
    int explosion_chance_cold = 0;
    float explosion_factor = 0.0f;
    bool fiery_explosion = false;
    float fuel_size_factor = 0.0f;
};

struct islot_fuel {
    public:
        /** Energy of the fuel (kilojoules per charge) */
        float energy = 0.0f;
        struct fuel_explosion explosion_data;
        bool has_explode_data = false;
        std::string pump_terrain = "t_null";
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
    std::string reload_noise = translate_marker( "click." );
    /**
     * Volume of the noise made when reloading this weapon.
     */
    int reload_noise_volume = 0;

    /** Maximum aim achievable using base weapon sights */
    int sight_dispersion = 30;

    /** Modifies base loudness as provided by the currently loaded ammo */
    int loudness = 0;

    /**
     * If this uses UPS charges, how many (per shoot), 0 for no UPS charges at all.
     */
    int ups_charges = 0;
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
    units::volume barrel_length = 0_ml;
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

    /** Burst size for AUTO mode (legacy field for items not migrated to specify modes ) */
    int burst = 0;

    /** How easy is control of recoil? If unset value automatically derived from weapon type */
    int handling = -1;

    /**
     *  Additional recoil applied per shot before effects of handling are considered
     *  @note useful for adding recoil effect to guns which otherwise consume no ammo
     */
    int recoil = 0;
};

/// The type of gun. The second "_type" suffix is only to distinguish it from `item::gun_type`.
class gun_type_type
{
    private:
        std::string name_;

    public:
        /// @param name The untranslated name of the gun type. Must have been extracted
        /// for translation with the context "gun_type_type".
        gun_type_type( const std::string &name ) : name_( name ) {}
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
    size_t operator()( const gun_type_type &t ) const {
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
     *  For sights (see @ref sight_dispersion), this value affects time cost of aiming.
     *  Higher is better. In case of multiple usable sights,
     *  the one with highest aim speed is used.
     */
    int aim_speed = -1;

    /** Modifies base loudness as provided by the currently loaded ammo */
    int loudness = 0;

    /** How many moves does this gunmod take to install? */
    int install_time = 0;

    /** Increases base gun UPS consumption by this many times per shot */
    float ups_charges_multiplier = 1.0f;

    /** Increases base gun UPS consumption by this value per shot */
    int ups_charges_modifier = 0;

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

    /** Modifies base strength required */
    int min_str_required_mod = 0;

    /** Additional gunmod slots to add to the gun */
    std::map<gunmod_location, int> add_mod;

    /** Not compatable on weapons that have this mod slot */
    std::set<gunmod_location> blacklist_mod;
};

struct islot_magazine {
    /** What type of ammo this magazine can be loaded with */
    std::set<ammotype> type;

    /** Capacity of magazine (in equivalent units to ammo charges) */
    int capacity = 0;

    /** Default amount of ammo contained by a magazine (often set for ammo belts) */
    int count = 0;

    /** Default type of ammo contained by a magazine (often set for ammo belts) */
    itype_id default_ammo = "NULL";

    /**
     * How reliable this magazine on a range of 0 to 10?
     * @see doc/GAME_BALANCE.md
     */
    int reliability = 0;

    /** How long it takes to load each unit of ammo into the magazine */
    int reload_time = 100;

    /** For ammo belts one linkage (of given type) is dropped for each unit of ammo consumed */
    cata::optional<itype_id> linkage;

    /** If false, ammo will cook off if this mag is affected by fire */
    bool protects_contents = false;
};

struct islot_battery {
    /** Maximum energy the battery can store */
    units::energy max_capacity;
};

struct islot_ammo : common_ranged_data {
    /**
     * Ammo type, basically the "form" of the ammo that fits into the gun/tool.
     */
    ammotype type;
    /**
     * Type id of casings, if any.
     */
    cata::optional<itype_id> casing;

    /**
     * Control chance for and state of any items dropped at ranged target
     *@{*/
    itype_id drop = "null";

    float drop_chance = 1.0;

    bool drop_active = true;
    /*@}*/

    /**
     * Default charges.
     */
    int def_charges = 1;

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
    float critical_multiplier = 2.0;

    /**
     * Some combat ammo might not have a damage value
     * Set this to make it show as combat ammo anyway
     */
    cata::optional<bool> force_stat_display;

    bool was_loaded;

    void load( const JsonObject &jo );
    void deserialize( JsonIn &jsin );
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
};

struct islot_seed {
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
     * Type id of the fruit item.
     */
    std::string fruit_id;
    /**
     * Whether to spawn seed items additionally to the fruit items.
     */
    bool spawn_seeds = true;
    /**
     * Additionally items (a list of their item ids) that will spawn when harvesting the plant.
     */
    std::vector<std::string> byproducts;

    islot_seed() = default;
};

struct islot_artifact {
    art_charge charge_type;
    art_charge_req charge_req;
    std::vector<art_effect_passive> effects_wielded;
    std::vector<art_effect_active>  effects_activated;
    std::vector<art_effect_passive> effects_carried;
    std::vector<art_effect_passive> effects_worn;
    std::vector<std::string> dream_msg_unmet;
    std::vector<std::string> dream_msg_met;
    int dream_freq_unmet;
    int dream_freq_met;
};

enum condition_type {
    FLAG,
    COMPONENT_ID,
    num_condition_types
};

template<>
struct enum_traits<condition_type> {
    static constexpr auto last = condition_type::num_condition_types;
};

// A name that is applied under certain conditions.
struct conditional_name {
    // Context type  (i.e. "FLAG"          or "COMPONENT_ID")
    condition_type type;
    // Context name  (i.e. "CANNIBALISM"   or "mutant")
    std::string condition;
    // Name to apply (i.e. "Luigi lasagne" or "smoked mutant"). Can use %s which will
    // be replaced by the item's normal name and/or preceding conditional names.
    translation name;
};

struct itype {
        friend class Item_factory;

        /**
         * Slots for various item type properties. Each slot may contain a valid pointer or null, check
         * this before using it.
         */
        /*@{*/
        cata::value_ptr<islot_container> container;
        cata::value_ptr<islot_tool> tool;
        cata::value_ptr<islot_comestible> comestible;
        cata::value_ptr<islot_brewable> brewable;
        cata::value_ptr<islot_armor> armor;
        cata::value_ptr<islot_pet_armor> pet_armor;
        cata::value_ptr<islot_book> book;
        cata::value_ptr<islot_mod> mod;
        cata::value_ptr<islot_engine> engine;
        cata::value_ptr<islot_wheel> wheel;
        cata::value_ptr<islot_fuel> fuel;
        cata::value_ptr<islot_gun> gun;
        cata::value_ptr<islot_gunmod> gunmod;
        cata::value_ptr<islot_magazine> magazine;
        cata::value_ptr<islot_battery> battery;
        cata::value_ptr<islot_bionic> bionic;
        cata::value_ptr<islot_ammo> ammo;
        cata::value_ptr<islot_seed> seed;
        cata::value_ptr<islot_artifact> artifact;
        cata::value_ptr<relic> relic_data;
        /*@}*/

    private:
        /** Can item be combined with other identical items? */
        bool stackable_ = false;

        /** Minimum and maximum amount of damage to an item (state of maximum repair). */
        // TODO: create and use a MinMax class or similar to put both values into one object.
        /// @{
        int damage_min_ = -1000;
        int damage_max_ = +4000;
        /// @}

    protected:
        std::string id = "null"; /** unique string identifier for this type */

        // private because is should only be accessed through itype::nname!
        // nname() is used for display purposes
        translation name = no_translation( "none" );

    public:
        itype() {
            melee.fill( 0 );
        }

        int damage_min() const {
            return count_by_charges() ? 0 : damage_min_;
        }
        int damage_max() const {
            return count_by_charges() ? 0 : damage_max_;
        }

        // used for generic_factory for copy-from
        bool was_loaded = false;

        // a hint for tilesets: if it doesn't have a tile, what does it look like?
        std::string looks_like;

        // What item this item repairs like if it doesn't have a recipe
        itype_id repairs_like;

        std::string snippet_category;
        translation description; // Flavor text
        ascii_art_id picture_id;

        // The container it comes in
        cata::optional<itype_id> default_container;

        std::map<quality_id, int> qualities; //Tool quality indicators
        std::map<std::string, std::string> properties;

        // A list of conditional names, in order of ascending priority.
        std::vector<conditional_name> conditional_names;

        // What we're made of (material names). .size() == made of nothing.
        // MATERIALS WORK IN PROGRESS.
        std::vector<material_id> materials;

        /** Actions an instance can perform (if any) indexed by action type */
        std::map<std::string, use_function> use_methods;

        /** Action to take BEFORE the item is placed on map. If it returns non-zero, item won't be placed. */
        use_function drop_action;

        /** Fields to emit when item is in active state */
        std::set<emit_id> emits;

        std::set<std::string> item_tags;
        std::set<matec_id> techniques;

        // Minimum stat(s) or skill(s) to use the item
        std::map<skill_id, int> min_skills;
        int min_str = 0;
        int min_dex = 0;
        int min_int = 0;
        int min_per = 0;

        phase_id phase      = SOLID; // e.g. solid, liquid, gas

        // How should the item explode
        explosion_data explosion;
        // Should the item explode when lit on fire
        bool explode_in_fire = false;

        /** Is item destroyed after the countdown action is run? */
        bool countdown_destroy = false;

        /** Default countdown interval (if any) for item */
        int countdown_interval = 0;

        /** Action to take when countdown expires */
        use_function countdown_action;

        /**
         * @name Non-negative properties
         * After loading from JSON these properties guaranteed to be zero or positive
         */
        /**@{*/

        /** Weight of item ( or each stack member ) */
        units::mass weight = 0_gram;
        /** Weight difference with the part it replaces for mods */
        units::mass integral_weight = -1_gram;

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

        /** Number of items per above volume for @ref count_by_charges items */
        int stack_size = 0;

        /** Value before cataclysm. Price given is for a default-sized stack. */
        units::money price = 0_cent;
        /** Value after cataclysm, dependent upon practical usages. Price given is for a default-sized stack. */
        units::money price_post = -1_cent;

        /**@}*/
        // If non-rigid volume (and if worn encumbrance) increases proportional to contents
        bool rigid = true;

        /** Damage output in melee for zero or more damage types */
        std::array<int, NUM_DT> melee;
        /** Base damage output when thrown */
        damage_instance thrown_damage;

        int m_to_hit  = 0;  // To-hit bonus for melee combat; -5 to 5 is reasonable

        unsigned light_emission = 0;   // Exactly the same as item_tags LIGHT_*, this is for lightmap.

        /** If set via JSON forces item category to this (preventing automatic assignment) */
        item_category_id category_force;

        std::string sym;
        nc_color color = c_white; // Color on the map (color.h)

        static constexpr int damage_scale = 1000; /** Damage scale compared to the old float damage value */

        /** What items can be used to repair this item? @see Item_factory::finalize */
        std::set<itype_id> repair;

        /** What recipes can make this item */
        std::vector<recipe_id> recipes;

        /** What faults (if any) can occur */
        std::set<fault_id> faults;

        /** Magazine types (if any) for each ammo type that can be used to reload this item */
        std::map< ammotype, std::set<itype_id> > magazines;

        /** Default magazine for each ammo type that can be used to reload this item */
        std::map< ammotype, itype_id > magazine_default;

        /** Volume above which the magazine starts to protrude from the item and add extra volume */
        units::volume magazine_well = 0_ml;

        layer_level layer = layer_level::MAX_CLOTHING_LAYER;

        /**
         * How much insulation this item provides, either as a container, or as
         * a vehicle base part.  Larger means more insulation, less than 1 but
         * greater than zero, transfers faster, cannot be less than zero.
         */
        float insulation_factor = 1;

        /**
         * Efficiency of solar energy conversion for solarpacks.
         */
        float solar_efficiency = 0;

        std::string get_item_type_string() const {
            if( tool ) {
                return "TOOL";
            } else if( comestible ) {
                return "FOOD";
            } else if( container ) {
                return "CONTAINER";
            } else if( armor ) {
                return "ARMOR";
            } else if( book ) {
                return "BOOK";
            } else if( gun ) {
                return "GUN";
            } else if( bionic ) {
                return "BIONIC";
            } else if( ammo ) {
                return "AMMO";
            }
            return "misc";
        }

        // Returns the name of the item type in the correct language and with respect to its grammatical number,
        // based on quantity (example: item type “anvil”, nname(4) would return “anvils” (as in “4 anvils”).
        std::string nname( unsigned int quantity ) const;

        // Allow direct access to the type id for the few cases that need it.
        itype_id get_id() const {
            return id;
        }

        bool count_by_charges() const {
            return stackable_ || ammo || comestible;
        }

        int charges_default() const {
            if( tool ) {
                return tool->def_charges;
            } else if( comestible ) {
                return comestible->def_charges;
            } else if( ammo ) {
                return ammo->def_charges;
            }
            return count_by_charges() ? 1 : 0;
        }

        int charges_to_use() const {
            if( tool ) {
                return static_cast<int>( tool->charges_per_use );
            }
            return 1;
        }

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
        bool can_use( const std::string &iuse_name ) const;
        const use_function *get_use( const std::string &iuse_name ) const;

        // Here "invoke" means "actively use". "Tick" means "active item working"
        int invoke( player &p, item &it, const tripoint &pos ) const; // Picks first method or returns 0
        int invoke( player &p, item &it, const tripoint &pos, const std::string &iuse_name ) const;
        int tick( player &p, item &it, const tripoint &pos ) const;

        virtual ~itype() = default;
};

#endif // CATA_SRC_ITYPE_H
