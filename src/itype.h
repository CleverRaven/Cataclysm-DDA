#ifndef ITYPE_H
#define ITYPE_H

#include "color.h" // nc_color
#include "enums.h" // point
#include "iuse.h" // use_function
#include "pldata.h" // add_type
#include "bodypart.h" // body_part::num_bp
#include "string_id.h"
#include "explosion.h"
#include "vitamin.h"

#include <string>
#include <vector>
#include <set>
#include <map>
#include <bitset>
#include <memory>

// see item.h
class item_category;
struct recipe;
struct itype;
class Skill;
using skill_id = string_id<Skill>;
class player;
class item;
class ma_technique;
using matec_id = string_id<ma_technique>;
enum art_effect_active : int;
enum art_charge : int;
enum art_effect_passive : int;
class material_type;
using material_id = string_id<material_type>;
typedef std::string itype_id;
typedef std::string ammotype;
class fault;
using fault_id = string_id<fault>;
struct quality;
using quality_id = string_id<quality>;

enum bigness_property_aspect : int {
    BIGNESS_WHEEL_DIAMETER      // wheel size in inches, including tire
};

// Returns the name of a category of ammo (e.g. "shot")
std::string ammo_name(std::string const &t);
// Returns the default ammo for a category of ammo (e.g. ""00_shot"")
std::string const& default_ammo(std::string const &guntype);

struct islot_tool {
    std::string ammo_id = "NULL";

    itype_id revert_to = "null";
    std::string revert_msg;

    std::string subtype;

    long max_charges = 0;
    long def_charges = 0;
    unsigned char charges_per_use = 0;
    unsigned char turns_per_charge = 0;
};

struct islot_comestible
{
    /** subtype, eg. FOOD, DRINK, MED */
    std::string comesttype;

     /** tool needed to consume (e.g. lighter for cigarettes) */
    std::string tool = "null";

    /** Defaults # of charges (drugs, loaf of bread? etc) */
    long def_charges = 1;

    /** effect on character thirst (may be negative) */
    int quench = 0;

    /** effect on character nutrition (may be negative) */
    int nutr = 0;

    /** turns until becomes rotten, or zero if never spoils */
    int spoils = 0;

    /** addiction potential */
    int addict = 0;

    /** effects of addiction */
    add_type add = ADD_NULL;

    /** effect on morale when consuming */
    int fun = 0;

    /** stimulant effect */
    int stim = 0;

    /** @todo add documentation */
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
};

struct islot_brewable {
    /** What are the results of fermenting this item? */
    std::vector<std::string> results;

    /** How many turns for this brew to ferment */
    int time = 0;
};

struct islot_container {
    /**
     * Volume, scaled by the default-stack size of the item that is contained in this container.
     */
    int contains = 0;
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
};

struct islot_armor {
    /**
     * Bitfield of enum body_part
     * TODO: document me.
     */
    std::bitset<num_bp> covers;
    /**
     * Whether this item can be worn on either side of the body
     */
    bool sided = false;
    /**
     * How much this item encumbers the player.
     */
    signed char encumber = 0;
    /**
     * Percentage of the body part area that this item covers.
     * This determines how likely it is to hit the item instead of the player.
     */
    unsigned char coverage = 0;
    /**
     * TODO: document me.
     */
    unsigned char thickness = 0;
    /**
     * Resistance to environmental effects.
     */
    unsigned char env_resist = 0;
    /**
     * How much warmth this item provides.
     */
    signed char warmth = 0;
    /**
     * How much storage this items provides when worn.
     */
    unsigned char storage = 0;
    /**
     * Whether this is a power armor item.
     */
    bool power_armor = false;
};

struct islot_book {
    /**
     * Which skill it upgrades, if any. Can be @ref skill_id::NULL_ID.
     */
    skill_id skill = NULL_ID;
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
        const struct recipe *recipe;
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
    /**
     * Special effects that can happen after the item has been read. May be empty.
     */
    std::vector<use_function> use_methods;
};

/**
 * Common data for ranged things: guns, gunmods and ammo.
 * The values of the gun itself, its mods and its current ammo (optional) are usually summed
 * up in the item class and the sum is used.
 */
struct common_ranged_data {
    /**
     * Armor-pierce bonus from gun.
     */
    int pierce = 0;
    /**
     * Range bonus from gun.
     */
    int range = 0;
    /**
     * Damage bonus from gun.
     */
    int damage = 0;
    /**
     * Dispersion "bonus" from gun.
     */
    int dispersion = 0;
    /**
     * Recoil "bonus" from gun.
     */
    int recoil = 0;
};

/**
 * Common data for things that affect firing: guns and gunmods.
 * The values of the gun itself and its mods are usually summed up in the item class
 * and the sum is used.
 */
struct common_firing_data : common_ranged_data {
    /**
     * TODO: this needs documentation, who knows what it is?
     * A value of -1 in gunmods means it's ignored.
     */
    int sight_dispersion = 0;
    /**
     * TODO: this needs documentation, who knows what it is?
     * A value of -1 in gunmods means it's ignored.
     */
    int aim_speed = 0;
    /**
     * Burst size.
     */
    int burst = 0;

    /** Modifies base loudness as provided by the currently loaded ammo */
    int loudness = 0;
};

struct islot_engine
{
    /** for combustion engines the displacement (cc) */
    int displacement = 0;

    /** What faults (if any) can occur */
    std::set<fault_id> faults;
};

// TODO: this shares a lot with the ammo item type, merge into a separate slot type?
struct islot_gun : common_firing_data {
    /**
     * What skill this gun uses.
     */
    skill_id skill_used = NULL_ID;
    /**
     * What type of ammo this gun uses.
     */
    ammotype ammo = "NULL";
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
    std::string reload_noise = "click.";
    /**
     * Volume of the noise made when reloading this weapon.
     */
    int reload_noise_volume = 0;
    /**
     * If this uses UPS charges, how many (per shoot), 0 for no UPS charges at all.
     */
    int ups_charges = 0;
    /**
     * Length of gun barrel, if positive allows sawing down of the barrel
     */
    int barrel_length = 0;
    /**
     * Effects that are applied to the ammo when fired.
     */
    std::set<std::string> ammo_effects;
    /**
     * Location for gun mods.
     * Key is the location (untranslated!), value is the number of mods
     * that the location can have. The value should be > 0.
     */
    std::map<std::string, int> valid_mod_locations;
    /**
    *Built in mods. string is id of mod. These mods will get the IRREMOVABLE flag set.
    */
    std::set<itype_id> built_in_mods;
    /**
    *Default mods, string is id of mod. These mods are removable but are default on the weapon.
    */
    std::set<itype_id> default_mods;
};

struct islot_gunmod : common_firing_data {
    /** Where is this guunmod installed (eg. "stock", "rail")? */
    std::string location;

    /** What kind of weapons can this gunmod be used with (eg. "rifle", "crossbow")? */
    std::set<std::string> usable;

    /** If non-empty restrict mod to guns with those base (before modifiers) ammo types */
    std::set<ammotype> acceptable_ammo;

    /** If changed from the default of "NULL" modifies parent guns ammo to this type */
    ammotype ammo_modifier = "NULL";

    /** How many moves does this gunmod take to install? */
    int install_time = 0;

    /** Increases base gun UPS consumption by this many charges per shot */
    int ups_charges = 0;

    /** If non-empty replaces the compatible magazines for the base gun */
    std::map< ammotype, std::set<itype_id> > magazine_adaptor;
};

struct islot_magazine {
    /** What type of ammo this magazine can be loaded with */
    std::string type = "NULL";

    /** Capacity of magazine (in equivalent units to ammo charges) */
    int capacity = 0;

    /** Default amount of ammo contained by a magazine (often set for ammo belts) */
    int count = 0;

    /**
     * How reliable this this magazine on a range of 0 to 10?
     * @see doc/GAME_BALANCE.md
     */
    int reliability = 0;

    /** How long it takes to load each unit of ammo into the magazine */
    int reload_time = 100;

    /** For ammo belts one linkage (of given type) is dropped for each unit of ammo consumed */
     itype_id linkage = "NULL";
};

struct islot_ammo : common_ranged_data {
    /**
     * Ammo type, basically the "form" of the ammo that fits into the gun/tool.
     * This is an id, it can be looked up in the @ref ammunition_type class.
     */
    std::string type;
    /**
     * Type id of casings, can be "null" for no casings at all.
     */
    std::string casing = "null";
    /**
     * Default charges.
     */
    long def_charges = 1;
    /**
     * TODO: document me.
     */
    std::set<std::string> ammo_effects;
    /**
     * Base loudness of ammo (possbily modified by gun/gunmods). If unspecified an
     * appropriate value is calculated based upon the other properties of the ammo
     */
    int loudness = -1;
};

struct islot_variable_bigness {
    /**
     * Minimal value of the bigness value of items of this type.
     */
    int min_bigness = 0;
    /**
     * Maximal value of the bigness value of items of this type.
     */
    int max_bigness = 0;
    /**
     * What the bigness actually represent see @ref bigness_property_aspect
     */
    bigness_property_aspect bigness_aspect = BIGNESS_WHEEL_DIAMETER;
};

struct islot_bionic {
    /**
     * Arbitrary difficulty scale, see bionics.cpp for its usage.
     */
    int difficulty = 0;
    /**
     * Id of the bionic, see @ref bionics.
     */
    std::string bionic_id;
};

struct islot_seed {
    /**
     * Time it takes for a seed to grow (in days, based of off a season length of 91)
     */
    int grow = 0;
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

// Data used when spawning items, should be obsoleted by the spawn system, but
// is still used at several places and makes it easier when it applies to all new items of a type.
struct islot_spawn {
    std::vector<long> rand_charges;
};

struct islot_artifact {
    art_charge charge_type;
    std::vector<art_effect_passive> effects_wielded;
    std::vector<art_effect_active>  effects_activated;
    std::vector<art_effect_passive> effects_carried;
    std::vector<art_effect_passive> effects_worn;
};

template <typename T>
class copyable_unique_ptr : public std::unique_ptr<T> {
    public:
        copyable_unique_ptr() = default;
        copyable_unique_ptr( copyable_unique_ptr&& rhs ) = default;

        copyable_unique_ptr( const copyable_unique_ptr<T>& rhs )
            : std::unique_ptr<T>( rhs ? new T( *rhs ) : nullptr ) {}
};

struct itype {
    friend class Item_factory;

    // unique string identifier for this item,
    // can be used as lookup key in master itype map
    // Used for save files; aligns to itype_id above.
    std::string id;
    /**
     * Slots for various item type properties. Each slot may contain a valid pointer or null, check
     * this before using it.
     */
    /*@{*/
    copyable_unique_ptr<islot_container> container;
    copyable_unique_ptr<islot_tool> tool;
    copyable_unique_ptr<islot_comestible> comestible;
    copyable_unique_ptr<islot_brewable> brewable;
    copyable_unique_ptr<islot_armor> armor;
    copyable_unique_ptr<islot_book> book;
    copyable_unique_ptr<islot_engine> engine;
    copyable_unique_ptr<islot_gun> gun;
    copyable_unique_ptr<islot_gunmod> gunmod;
    copyable_unique_ptr<islot_magazine> magazine;
    copyable_unique_ptr<islot_variable_bigness> variable_bigness;
    copyable_unique_ptr<islot_bionic> bionic;
    copyable_unique_ptr<islot_spawn> spawn;
    copyable_unique_ptr<islot_ammo> ammo;
    copyable_unique_ptr<islot_seed> seed;
    copyable_unique_ptr<islot_artifact> artifact;
    /*@}*/
protected:
    // private because is should only be accessed through itype::nname!
    // name and name_plural are not translated automatically
    // nname() is used for display purposes
    std::string name;        // Proper name, singular form, in American English.
    std::string name_plural; // name, plural form, in American English.
public:
    std::string snippet_category;
    std::string description; // Flavor text

    std::string default_container = "null"; // The container it comes in

    std::map<quality_id, int> qualities; //Tool quality indicators
    std::map<std::string, std::string> properties;

    // What we're made of (material names). .size() == made of nothing.
    // MATERIALS WORK IN PROGRESS.
    std::vector<material_id> materials;
    std::vector<use_function> use_methods; // Special effects of use

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

    /** After loading from JSON these properties guaranteed to be zero or positive */
    /*@{*/
    int weight          =  0; // Weight in grams for item (or each stack member)
    int volume          =  0; // Space occupied by items of this type
    int price           =  0; // Value before cataclysm
    int price_post      = -1; // Value after cataclysm (dependent upon practical usages)
    int stack_size      =  0; // Maximum identical items that can stack per above unit volume
    int integral_volume = -1; // Space consumed when integrated as part of another item (defaults to volume)
    /*@}*/

    bool rigid = true; // If non-rigid volume (and if worn encumbrance) increases proportional to contents

    int melee_dam = 0; // Bonus for melee damage; may be a penalty
    int melee_cut = 0; // Cutting damage in melee
    int m_to_hit  = 0;  // To-hit bonus for melee combat; -5 to 5 is reasonable

    unsigned light_emission = 0;   // Exactly the same as item_tags LIGHT_*, this is for lightmap.

    const item_category *category = nullptr; // category pointer or NULL for automatic selection

    nc_color color = c_white; // Color on the map (color.h)
    char sym = 0; // Symbol on the map

    /** Magazine types (if any) for each ammo type that can be used to reload this item */
    std::map< ammotype, std::set<itype_id> > magazines;

    /** Default magazine for each ammo type that can be used to reload this item */
    std::map< ammotype, itype_id > magazine_default;

    /** Volume above which the magazine starts to protrude from the item and add extra volume */
    int magazine_well = 0;

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
        } else if( variable_bigness ) {
            return "VEHICLE_PART";
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

    bool count_by_charges() const
    {
        if( ammo ) {
            return true;
        } else if( comestible ) {
            return phase == LIQUID || comestible->def_charges > 1 || stack_size > 1;
        }
        return false;
    }

    int charges_default() const {
        if( tool ) {
            return tool->def_charges;
        } else if( comestible ) {
            return comestible->def_charges;
        } else if( ammo ) {
            return ammo->def_charges;
        }
        return 0;
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
    long invoke( player *p, item *it, const tripoint &pos ) const; // Picks first method or returns 0
    long invoke( player *p, item *it, const tripoint &pos, const std::string &iuse_name ) const;
    long tick( player *p, item *it, const tripoint &pos ) const;

    itype() : id("null"), name("none"), name_plural("none") {}

    virtual ~itype() { };
};

#endif

