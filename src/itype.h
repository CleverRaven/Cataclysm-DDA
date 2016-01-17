#ifndef ITYPE_H
#define ITYPE_H

#include "color.h" // nc_color
#include "enums.h" // point
#include "iuse.h" // use_function
#include "pldata.h" // add_type
#include "bodypart.h" // body_part::num_bp
#include "string_id.h"

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

typedef std::string itype_id;
typedef std::string ammotype;

enum bigness_property_aspect : int {
    BIGNESS_ENGINE_DISPLACEMENT, // combustion engine CC displacement
    BIGNESS_WHEEL_DIAMETER,      // wheel size in inches, including tire
};

// Returns the name of a category of ammo (e.g. "shot")
std::string ammo_name(std::string const &t);
// Returns the default ammo for a category of ammo (e.g. ""00_shot"")
std::string const& default_ammo(std::string const &guntype);

struct explosion_data {
    // Those 4 values are forwarded to game::explosion.
    float power           = -1.0f;
    float distance_factor = 0.8f;
    int shrapnel          = 0;
    bool fire             = false;
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
    /**
     * Volume of the item does not include volume of the content.
     */
    bool rigid = false;
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
    /**
     * Clip size. Note that on some gunmods it means relative (in percent) of the
     * guns main magazine.
     */
    int clip = 0;
    /**
     * loudness for guns/gunmods
     */
    int loudness = 0;
};

// TODO: this shares a lot with the ammo item type, merge into a separate slot type?
struct islot_gun : common_firing_data {
    /**
     * What type of ammo this gun uses.
     */
    std::string ammo;
    /**
     * What skill this gun uses.
     * TODO: This is also indicates the type of gun (handgun/rifle/etc.) - that
     * should probably be made explicit.
     */
    skill_id skill_used = NULL_ID;
    /**
     * Gun durability, affects gun being damaged during shooting.
     */
    int durability = 0;
    /**
     * Reload time, in moves.
     */
    int reload_time = 0;
    /**
     * Noise displayed when reloading the weapon.
     */
    std::string reload_noise;
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
    std::vector<std::string> built_in_mods;
    /**
    *Default mods, string is id of mod. These mods are removable but are default on the weapon.
    */
    std::vector<std::string> default_mods;
};

struct islot_gunmod : common_firing_data {
    /**
     * TODO: document me
     */
    int req_skill = 0;
    /**
     * TODO: document me
     */
    skill_id skill_used = NULL_ID;
    /**
     * TODO: document me
     */
    std::string newtype;
    /**
     * TODO: document me
     */
    std::string location;
    /**
     * TODO: document me
     */
    std::set<std::string> acceptable_ammo_types;
    /**
     * TODO: document me
     */
    bool used_on_pistol = false;
    /**
     * TODO: document me
     */
    bool used_on_shotgun = false;
    /**
     * TODO: document me
     */
    bool used_on_smg = false;
    /**
     * TODO: document me
     */
    bool used_on_rifle = false;
    /**
     * TODO: document me
     */
    bool used_on_bow = false;
    /**
     * TODO: document me
     */
    bool used_on_crossbow = false;
    /**
     * TODO: document me
     */
    bool used_on_launcher = false;
    /**
    *Allowing a mod to add UPS charge requirement to a gun.
    */
    int ups_charges = 0;
};

struct islot_magazine {
    /**
     * What type of ammo this magazine can be loaded with
     */
    std::string type;
    /**
     * Capacity of magazine (in equivalent units to ammo charges)
     */
    int capacity;
    /**
     * Percentage chance each round is fed without causing a jam
     */
    int reliability;
    /**
     * How long it takes to load each unit of ammo into the magazine
     */
    int reload_time;
};

struct islot_ammo : common_ranged_data {
    /**
     * Ammo type, basically the "form" of the ammo that fits into the gun/tool.
     * This is an id, it can be looked up in the @ref ammunition_type class.
     */
    std::string type;
    /**
     * Type id of casings, can be "NULL" for no casings at all.
     */
    std::string casing;
    /**
     * Default charges.
     */
    long def_charges = 0;
    /**
     * TODO: document me.
     */
    std::set<std::string> ammo_effects;

    islot_ammo() : casing ("NULL") { }
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
    bigness_property_aspect bigness_aspect = BIGNESS_ENGINE_DISPLACEMENT;
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

struct islot_software {
    /**
     * Type of software, not used by anything at all.
     */
    std::string type = "USELESS";
    /**
     * No used, but it's there is the original data.
     */
    int power;
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
    std::string default_container; // The container it comes in
    std::vector<long> rand_charges;

    islot_spawn() : default_container ("null") { }
};

struct islot_artifact {
    art_charge charge_type;
    std::vector<art_effect_passive> effects_wielded;
    std::vector<art_effect_active>  effects_activated;
    std::vector<art_effect_passive> effects_carried;
    std::vector<art_effect_passive> effects_worn;
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
    std::unique_ptr<islot_container> container;
    std::unique_ptr<islot_armor> armor;
    std::unique_ptr<islot_book> book;
    std::unique_ptr<islot_gun> gun;
    std::unique_ptr<islot_gunmod> gunmod;
    std::unique_ptr<islot_magazine> magazine;
    std::unique_ptr<islot_variable_bigness> variable_bigness;
    std::unique_ptr<islot_bionic> bionic;
    std::unique_ptr<islot_software> software;
    std::unique_ptr<islot_spawn> spawn;
    std::unique_ptr<islot_ammo> ammo;
    std::unique_ptr<islot_seed> seed;
    std::unique_ptr<islot_artifact> artifact;
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

    std::map<std::string, int> qualities; //Tool quality indicators
    std::map<std::string, std::string> properties;

    // What we're made of (material names). .size() == made of nothing.
    // MATERIALS WORK IN PROGRESS.
    std::vector<std::string> materials;
    std::vector<use_function> use_methods; // Special effects of use

    std::set<std::string> item_tags;
    std::set<matec_id> techniques;

    // Minimum stat(s) or skill(s) to use the item
    int min_str = 0;
    int min_dex = 0;
    int min_int = 0;
    int min_per = 0;
    std::map<skill_id, int> min_skills;

    // Explosion that happens when the item is set on fire
    explosion_data explosion_on_fire_data;

    phase_id phase      = SOLID; // e.g. solid, liquid, gas
    unsigned price      = 0; // Its value
    unsigned volume     = 0; // Space taken up by this item
    int      stack_size = 0; // How many things make up the above-defined volume (eg. 100 aspirin = 1 volume)
    unsigned weight     = 0; // Weight in grams. Assumes positive weight. No helium, guys!

    int melee_dam = 0; // Bonus for melee damage; may be a penalty
    int melee_cut = 0; // Cutting damage in melee
    int m_to_hit  = 0;  // To-hit bonus for melee combat; -5 to 5 is reasonable

    unsigned light_emission = 0;   // Exactly the same as item_tags LIGHT_*, this is for lightmap.

    const item_category *category = nullptr; // category pointer or NULL for automatic selection

    nc_color color = c_white; // Color on the map (color.h)
    char sym = '#';       // Symbol on the ma

    bool explode_in_fire() const
    {
        return explosion_on_fire_data.power >= 0;
    }

    virtual std::string get_item_type_string() const
    {
        if( container ) {
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
    virtual std::string nname(unsigned int quantity) const;

    virtual bool is_food() const
    {
        return false;
    }

    virtual bool is_tool() const
    {
        return false;
    }

    virtual bool count_by_charges() const
    {
        if( ammo ) {
            return true;
        }
        return false;
    }

    virtual int charges_to_use() const
    {
        return 1;
    }

    virtual int maximum_charges() const
    {
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

    itype(std::string pid, unsigned pprice, std::string pname, std::string pname_plural,
          std::string pdes, char psym, nc_color pcolor, std::vector<std::string> pmaterials,
          phase_id pphase, unsigned pvolume, unsigned pweight, int pmelee_dam,
          int pmelee_cut, int pm_to_hit) : id(std::move(pid)), name(std::move(pname)),
          name_plural(std::move(pname_plural)), description(std::move(pdes)),
          materials(std::move(pmaterials)), phase(pphase), price(pprice), volume(pvolume),
          weight(pweight), melee_dam(pmelee_dam), melee_cut(pmelee_cut), m_to_hit(pm_to_hit),
          color(pcolor), sym(psym) { }

    virtual ~itype() { };
};

// Includes food drink and drugs
struct it_comest : itype {
    friend class Item_factory;

    std::string tool;       // Tool needed to consume (e.g. lighter for cigarettes)
    std::string comesttype; // FOOD, DRINK, MED
    long        def_charges = 0;  // Defaults # of charges (drugs, loaf of bread? etc)
    int         quench      = 0;  // Many things make you thirstier!
private:
    // Both values are kept to ease migration
    // Negative nutr means it is unset - use calories
    int         nutr        = 0;  // Nutrition imparted
    int         kcal        = 0;  // Replacement for the above
public:
    /**
     * How long it takes to spoil (turns), rotten food is handled differently
     * (chance of bad thinks happen when eating etc).
     * If 0, the food never spoils.
     */
    int      spoils   = 0;
    unsigned addict   = 0; // Addictiveness potential
    int      stim     = 0;
    int      healthy  = 0;
    unsigned brewtime = 0; // How long it takes for a brew to ferment.
    int      fun      = 0; // How fun its use is

    add_type add = ADD_NULL; // Effects of addiction

    it_comest() = default;

    bool is_food() const override
    {
        return true;
    }

    std::string get_item_type_string() const override
    {
        return "FOOD";
    }

    bool count_by_charges() const override
    {
        if (phase == LIQUID) {
            return true;
        } else {
            return def_charges > 1 ;
        }
    }

    int get_nutrition() const;

    int get_calories() const;
};

struct it_tool : itype {
    std::string ammo_id;
    std::string revert_to;
    std::string subtype;

    long max_charges = 0;
    long def_charges = 0;
    unsigned char charges_per_use = 0;
    unsigned char turns_per_charge = 0;

    it_tool() = default;

    bool is_tool() const override
    {
        return true;
    }

    std::string get_item_type_string() const override
    {
        return "TOOL";
    }

    int charges_to_use() const override
    {
        return charges_per_use;
    }

    int maximum_charges() const override
    {
        return max_charges;
    }
};

#endif

