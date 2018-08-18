#pragma once
#ifndef ITYPE_H
#define ITYPE_H

#include "optional.h"
#include "color.h" // nc_color
#include "enums.h" // point
#include "iuse.h" // use_function
#include "pldata.h" // add_type
#include "bodypart.h" // body_part::num_bp
#include "string_id.h"
#include "explosion.h"
#include "units.h"
#include "damage.h"
#include "translations.h"
#include "calendar.h"

#include <string>
#include <vector>
#include <set>
#include <map>
#include <bitset>
#include <memory>

// see item.h
class item_category;
class gun_mode;
using gun_mode_id = string_id<gun_mode>;
class Item_factory;
class recipe;
class emit;
using emit_id = string_id<emit>;
struct itype;
class Skill;
using skill_id = string_id<Skill>;
struct bionic_data;
using bionic_id = string_id<bionic_data>;
class player;
class item;
class vitamin;
using vitamin_id = string_id<vitamin>;
class ma_technique;
using matec_id = string_id<ma_technique>;
enum art_effect_active : int;
enum art_charge : int;
enum art_charge_req : int;
enum art_effect_passive : int;
struct artifact_dream_datum;
class material_type;
using material_id = string_id<material_type>;
typedef std::string itype_id;
class ammunition_type;
using ammotype = string_id<ammunition_type>;
class fault;
using fault_id = string_id<fault>;
struct quality;
using quality_id = string_id<quality>;
struct MonsterGroup;
using mongroup_id = string_id<MonsterGroup>;

enum field_id : int;

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
        gun_modifier_data( const std::string &n, const int q, const std::set<std::string> &f ) : name_( n ), qty_( q ), flags_( f ) { }
        /// @returns The translated name of the gun mode.
        std::string name() const {
            return _( name_.c_str() );
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
    ammotype ammo_id = ammotype::NULL_ID();

    itype_id revert_to = "null";
    std::string revert_msg;

    std::string subtype;

    long max_charges = 0;
    long def_charges = 0;
    std::vector<long> rand_charges;
    unsigned char charges_per_use = 0;
    unsigned char turns_per_charge = 0;
};

struct islot_comestible
{
    /** subtype, e.g. FOOD, DRINK, MED */
    std::string comesttype;

     /** tool needed to consume (e.g. lighter for cigarettes) */
    std::string tool = "null";

    /** Defaults # of charges (drugs, loaf of bread? etc) */
    long def_charges = 1;

    /** effect on character thirst (may be negative) */
    int quench = 0;

    /** effect on character nutrition (may be negative) */
    int nutr = 0;

    /** Time until becomes rotten at standard temperature, or zero if never spoils */
    time_duration spoils = 0;

    /** addiction potential */
    int addict = 0;

    /** effects of addiction */
    add_type add = ADD_NULL;

    /** effect on morale when consuming */
    int fun = 0;

    /** stimulant effect */
    int stim = 0;

    /** @todo: add documentation */
    int healthy = 0;

    /** chance (odds) of becoming parasitised when eating (zero if never occurs) */
    int parasites = 0;

    /** vitamins potentially provided by this comestible (if any) */
    std::map<vitamin_id, int> vitamins;

    /** 1 nutr ~= 8.7kcal (1 nutr/5min = 288 nutr/day at 2500kcal/day) */
    static constexpr float kcal_per_nutr = 2500.0f / ( 12 * 24 );

    int get_calories() const {
        return nutr * kcal_per_nutr;
    }
    /** The monster group that is drawn from when the item rots away */
    mongroup_id rot_spawn = mongroup_id::NULL_ID();

    /** Chance the above monster group spawns*/
    int rot_spawn_chance = 10;
};

struct islot_brewable {
    /** What are the results of fermenting this item? */
    std::vector<std::string> results;

    /** How long for this brew to ferment. */
    time_duration time = 0;
};

struct islot_container {
    /**
     * Inner volume of the container.
     */
    units::volume contains = 0;
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
    units::volume storage = 0;
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
     * How long, in 10-turns (aka minutes), it takes to read.
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
        bool operator<( const recipe_with_description_t &rhs ) const
        {
            return recipe < rhs.recipe;
        }
        bool is_hidden() const
        {
            return hidden;
        }
    };
    typedef std::set<recipe_with_description_t> recipe_list_t;
    recipe_list_t recipes;
};

struct islot_mod {
    /** If non-empty restrict mod to items with those base (before modifiers) ammo types */
    std::set<ammotype> acceptable_ammo;

    /** If set modifies parent ammo to this type */
    ammotype ammo_modifier = ammotype::NULL_ID();

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
    /**
     * Legacy pierce and damage values, used if @ref damage isn't set.
    *@{*/
    int legacy_pierce = 0;
    int legacy_damage = 0;
    /*@}*/
};

struct islot_engine
{
    friend Item_factory;
    friend item;

    public:
        /** for combustion engines the displacement (cc) */
        int displacement = 0;

    private:
        /** What faults (if any) can occur */
        std::set<fault_id> faults;
};

struct islot_wheel
{
    public:
        /** diameter of wheel (inches) */
        int diameter = 0;

        /** width of wheel (inches) */
        int width = 0;
};

struct islot_fuel
{
    public:
        /** Energy of the fuel (kilojoules per charge) */
        float energy = 0.0f;
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
    ammotype ammo = ammotype::NULL_ID();
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
     * Length of gun barrel, if positive allows sawing down of the barrel
     */
    units::volume barrel_length = 0;
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
        // arbitrary sorting, only here to allow usage in std::set
        bool operator<( const gun_type_type &rhs ) const {
            return name_ < rhs.name_;
        }
        /// Translated name.
        std::string name() const;
};

struct islot_gunmod : common_ranged_data {
    /** Where is this gunmod installed (e.g. "stock", "rail")? */
    gunmod_location location;

    /** What kind of weapons can this gunmod be used with (e.g. "rifle", "crossbow")? */
    std::set<gun_type_type> usable;

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

    /** Increases base gun UPS consumption by this many charges per shot */
    int ups_charges = 0;

    /** Firing modes added to or replacing those of the base gun */
    std::map<gun_mode_id, gun_modifier_data> mode_modifier;

    std::set<std::string> ammo_effects;

    /** Relative adjustment to base gun handling */
    int handling = 0;
};

struct islot_magazine {
    /** What type of ammo this magazine can be loaded with */
    ammotype type = ammotype::NULL_ID();

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
    itype_id linkage = "NULL";

    /** If false, ammo will cook off if this mag is affected by fire */
    bool protects_contents = false;
};

struct islot_ammo : common_ranged_data {
    /**
     * Ammo type, basically the "form" of the ammo that fits into the gun/tool.
     */
    std::set<ammotype> type;
    /**
     * Type id of casings, can be "null" for no casings at all.
     */
    std::string casing = "null";
    /**
     * Default charges.
     */

    /**
     * Control chance for and state of any items dropped at ranged target
     *@{*/
    itype_id drop = "null";

    float drop_chance = 1.0;

    bool drop_active = true;
    /*@}*/

    long def_charges = 1;
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
};

struct islot_seed {
    /**
     * Time it takes for a seed to grow (based of off a season length of 91 days).
     */
    time_duration grow = 0;
    /**
     * Amount of harvested charges of fruits is divided by this number.
     */
    int fruit_div = 1;
    /**
     * Name of the plant, already translated.
     */
    std::string plant_name;
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

    islot_seed() { }
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
bool check_art_charge_req( item& it );

struct itype {
    friend class Item_factory;

    /**
     * Slots for various item type properties. Each slot may contain a valid pointer or null, check
     * this before using it.
     */
    /*@{*/
    cata::optional<islot_container> container;
    cata::optional<islot_tool> tool;
    cata::optional<islot_comestible> comestible;
    cata::optional<islot_brewable> brewable;
    cata::optional<islot_armor> armor;
    cata::optional<islot_book> book;
    cata::optional<islot_mod> mod;
    cata::optional<islot_engine> engine;
    cata::optional<islot_wheel> wheel;
    cata::optional<islot_fuel> fuel;
    cata::optional<islot_gun> gun;
    cata::optional<islot_gunmod> gunmod;
    cata::optional<islot_magazine> magazine;
    cata::optional<islot_bionic> bionic;
    cata::optional<islot_ammo> ammo;
    cata::optional<islot_seed> seed;
    cata::optional<islot_artifact> artifact;
    /*@}*/

protected:
    std::string id = "null"; /** unique string identifier for this type */

    // private because is should only be accessed through itype::nname!
    // name and name_plural are not translated automatically
    // nname() is used for display purposes
    std::string name = "none";        // Proper name, singular form, in American English.
    std::string name_plural = "none"; // name, plural form, in American English.

    /** If set via JSON forces item category to this (preventing automatic assignment) */
    std::string category_force;

public:
    itype() {
        melee.fill( 0 );
    }

    // a hint for tilesets: if it doesn't have a tile, what does it look like?
    std::string looks_like;

    std::string snippet_category;
    std::string description; // Flavor text

    std::string default_container = "null"; // The container it comes in

    std::map<quality_id, int> qualities; //Tool quality indicators
    std::map<std::string, std::string> properties;

    // What we're made of (material names). .size() == made of nothing.
    // MATERIALS WORK IN PROGRESS.
    std::vector<material_id> materials;

    /** Actions an instance can perform (if any) indexed by action type */
    std::map<std::string, use_function> use_methods;

    /** Default countdown interval (if any) for item */
    int countdown_interval = 0;

    /** Action to take when countdown expires */
    use_function countdown_action;

    /** Is item destroyed after the countdown action is run? */
    bool countdown_destroy = false;

    /** Action to take BEFORE the item is placed on map. If it returns non-zero, item won't be placed. */
    use_function drop_action;

    /** Fields to emit when item is in active state */
    std::set<emit_id> emits;

    std::set<std::string> item_tags;
    std::set<matec_id> techniques;

    // Minimum stat(s) or skill(s) to use the item
    int min_str = 0;
    int min_dex = 0;
    int min_int = 0;
    int min_per = 0;
    std::map<skill_id, int> min_skills;

    // Should the item explode when lit on fire
    bool explode_in_fire = false;
    // How should the item explode
    explosion_data explosion;

    phase_id phase      = SOLID; // e.g. solid, liquid, gas

    /** Can item be combined with other identical items? */
    bool stackable = false;

    /** After loading from JSON these properties guaranteed to be zero or positive */
    /*@{*/
    units::mass weight  =  0; // Weight for item ( or each stack member )
    units::volume volume = 0; // Space occupied by items of this type
    int price           =  0; // Value before cataclysm
    int price_post      = -1; // Value after cataclysm (dependent upon practical usages)
    int stack_size      =  0; // Maximum identical items that can stack per above unit volume
    units::volume integral_volume = units::from_milliliter( -1 ); // Space consumed when integrated as part of another item (defaults to volume)
    /*@}*/

    bool rigid = true; // If non-rigid volume (and if worn encumbrance) increases proportional to contents

    /** Damage output in melee for zero or more damage types */
    std::array<int, NUM_DT> melee;
    /** Base damage output when thrown */
    damage_instance thrown_damage;

    int m_to_hit  = 0;  // To-hit bonus for melee combat; -5 to 5 is reasonable

    unsigned light_emission = 0;   // Exactly the same as item_tags LIGHT_*, this is for lightmap.

    const item_category *category = nullptr; // category pointer or NULL for automatic selection

    nc_color color = c_white; // Color on the map (color.h)
    std::string sym;

    int damage_min = -1; /** Minimum amount of damage to an item (state of maximum repair) */
    int damage_max =  4; /** Maximum amount of damage to an item (state before destroyed) */

    /** What items can be used to repair this item? @see Item_factory::finalize */
    std::set<itype_id> repair;

    /** Magazine types (if any) for each ammo type that can be used to reload this item */
    std::map< ammotype, std::set<itype_id> > magazines;

    /** Default magazine for each ammo type that can be used to reload this item */
    std::map< ammotype, itype_id > magazine_default;

    /** Volume above which the magazine starts to protrude from the item and add extra volume */
    units::volume magazine_well = 0;

    std::string get_item_type_string() const
    {
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
    std::string nname(unsigned int quantity) const;

    // Allow direct access to the type id for the few cases that need it.
    itype_id get_id() const {
        return id;
    }

    bool count_by_charges() const { return stackable; }

    int charges_default() const {
        if( tool ) {
            return tool->def_charges;
        } else if( comestible ) {
            return comestible->def_charges;
        } else if( ammo ) {
            return ammo->def_charges;
        }
        return stackable ? 1 : 0;
    }

    int charges_to_use() const
    {
        if( tool ) {
            return tool->charges_per_use;
        }
        return 1;
    }

    int maximum_charges() const
    {
        if( tool ) {
            return tool->max_charges;
        }
        return 1;
    }

    bool has_use() const;
    bool can_use( const std::string &iuse_name ) const;
    const use_function *get_use( const std::string &iuse_name ) const;

    // Here "invoke" means "actively use". "Tick" means "active item working"
    long invoke( player &p, item &it, const tripoint &pos ) const; // Picks first method or returns 0
    long invoke( player &p, item &it, const tripoint &pos, const std::string &iuse_name ) const;
    long tick( player &p, item &it, const tripoint &pos ) const;

    virtual ~itype() = default;
};

#endif
