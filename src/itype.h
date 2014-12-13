#ifndef ITYPE_H
#define ITYPE_H

#include "color.h"
#include "enums.h"
#include "iuse.h"
#include "pldata.h"
#include "bodypart.h"
#include "skill.h"
#include "bionics.h"
#include "rng.h"
#include "material.h"
#include "mtype.h"

#include <string>
#include <vector>
#include <set>
#include <bitset>
#include <memory>

typedef std::string itype_id;

// see item.h
class item_category;
struct recipe;
struct itype;

typedef std::string ammotype;

enum software_type {
    SW_NULL,
    SW_USELESS,
    SW_HACKING,
    SW_MEDICAL,
    SW_SCIENCE,
    SW_DATA,
    NUM_SOFTWARE_TYPES
};

enum bigness_property_aspect {
    BIGNESS_ENGINE_NULL,         // like a cookie-cutter-cut cookie, this type has no bigness aspect.
    BIGNESS_ENGINE_DISPLACEMENT, // combustion engine CC displacement
    BIGNESS_KILOWATTS,           // electric motor power
    BIGNESS_WHEEL_DIAMETER,      // wheel size in inches, including tire
    //BIGNESS_PLATING_THICKNESS, //
    NUM_BIGNESS_ASPECTS,
};

// Returns the name of a category of ammo (e.g. "shot")
std::string ammo_name(ammotype t);
// Returns the default ammo for a category of ammo (e.g. ""00_shot"")
itype_id default_ammo(ammotype guntype);

struct explosion_data {
    // Those 4 values are forwarded to game::explosion.
    int power;
    int shrapnel;
    bool fire;
    bool blast;
    explosion_data() : power(-1), fire(false), blast(true) { }
};

struct islot_container {
    /**
     * Volume, scaled by the default-stack size of the item that is contained in this container.
     */
    int contains;
    /**
     * Can be resealed.
     */
    bool seals;
    /**
     * Can hold liquids.
     */
    bool watertight;
    /**
     * Contents do not spoil.
     */
    bool preserves;
    /**
     * Volume of the item does not include volume of the content.
     */
    bool rigid;

    islot_container()
    : contains( 0 )
    , seals( false )
    , watertight( false )
    , preserves( false )
    , rigid( false )
    {
    }
};

struct islot_armor {
    /**
     * Bitfield of enum body_part
     * TODO: document me.
     */
    std::bitset<num_bp> covers;
    /**
     * Bitfield of enum body_part
     * TODO: document me.
     */
    std::bitset<num_bp> sided;
    /**
     * How much this item encumbers the player.
     */
    signed char encumber;
    /**
     * Percentage of the body part area that this item covers.
     * This determines how likely it is to hit the item instead of the player.
     */
    unsigned char coverage;
    /**
     * TODO: document me.
     */
    unsigned char thickness;
    /**
     * Resistance to environmental effects.
     */
    unsigned char env_resist;
    /**
     * How much warmth this item provides.
     */
    signed char warmth;
    /**
     * How much storage this items provides when worn.
     */
    unsigned char storage;
    /**
     * Whether this is a power armor item.
     */
    bool power_armor;

    islot_armor()
    : covers( 0 )
    , sided( 0 )
    , encumber( 0 )
    , coverage( 0 )
    , thickness( 0 )
    , env_resist( 0 )
    , warmth( 0 )
    , storage( 0 )
    , power_armor( false )
    {
    }
};

struct islot_book {
    /**
     * Which skill it upgrades, if any. Can be NULL.
     * TODO: this should be a pointer to const
     */
    Skill *skill;
    /**
     * The skill level the book provides.
     */
    int level;
    /**
     * The skill level required to understand it.
     */
    int req;
    /**
     * How fun reading this is, can be negative.
     */
    int fun;
    /**
     * Intelligence required to read it.
     */
    int intel;
    /**
     * How long, in 10-turns (aka minutes), it takes to read.
     * "To read" means getting 1 skill point, not all of them.
     */
    int time;
    /**
     * Fun books have chapters; after all are read, the book is less fun.
     */
    int chapters;
    /**
     * What recipes can be learned from this book.
     * Key is the recipe, value is skill level (of the main skill of the recipes) that is required
     * to learn the recipe.
     */
    std::map<const recipe *, int> recipes;
    /**
     * Special effects that can happen after the item has been read. May be empty.
     */
    std::vector<use_function> use_methods;

    islot_book()
    : skill( nullptr )
    , level( 0 )
    , req( 0 )
    , fun( 0 )
    , intel( 0 )
    , time( 0 )
    , chapters( 0 )
    , recipes()
    , use_methods()
    {
    }
};

// TODO: this shares a lot with the ammo item type, merge into a separate slot type?
struct islot_gun {
    /**
     * What type of ammo this gun uses.
     */
    ammotype ammo;
    /**
     * What skill this gun uses.
     * TODO: This is also indicates the type of gun (handgun/rifle/etc.) - that
     * should probably be made explicit.
     * TODO: this should be a pointer to a const Skill
     */
    Skill *skill_used;
    /**
     * Damage bonus from gun.
     */
    int dmg_bonus;
    /**
     * Armor-pierce bonus from gun.
     */
    int pierce;
    /**
     * Range bonus from gun.
     */
    int range;
    /**
     * Dispersion "bonus" from gun.
     */
    int dispersion;
    /**
     * TODO: document me
     */
    int sight_dispersion;
    /**
     * TODO: document me
     */
    int aim_speed;
    /**
     * Recoil "bonus" from gun.
     */
    int recoil;
    /**
     * Gun durability, affects gun being damaged during shooting.
     */
    int durability;
    /**
     * Burst size.
     */
    int burst;
    /**
     * Clip size.
     */
    int clip;
    /**
     * Reload time.
     */
    int reload_time;
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
     * If this uses UPS charges, how many (per shoot), 0 for no UPS charges at all.
     */
    int ups_charges;

    islot_gun()
    : skill_used( nullptr )
    , dmg_bonus( 0 )
    , pierce( 0 )
    , range( 0 )
    , dispersion( 0 )
    , sight_dispersion( 0 )
    , aim_speed( 0 )
    , recoil( 0 )
    , durability( 0 )
    , burst( 0 )
    , clip( 0 )
    , reload_time( 0 )
    , ammo_effects()
    , valid_mod_locations()
    , ups_charges( 0 )
    {
    }
};

struct itype {
    itype_id id; // unique string identifier for this item,
    // can be used as lookup key in master itype map
    // Used for save files; aligns to itype_id above.
    unsigned int  price; // Its value

    /**
     * Slots for various item type properties. Each slot may contain a valid pointer or null, check
     * this before using it.
     */
    /*@{*/
    std::unique_ptr<islot_container> container;
    std::unique_ptr<islot_armor> armor;
    std::unique_ptr<islot_book> book;
    std::unique_ptr<islot_gun> gun;
    /*@}*/

protected:
    friend class Item_factory;
    // private because is should only be accessed through itype::nname!
    // name and name_plural are not translated automatically
    // nname() is used for display purposes
    std::string name;        // Proper name, singular form, in American English.
    std::string name_plural; // name, plural form, in American English.
public:
    std::string description; // Flavor text

    char sym;       // Symbol on the map
    nc_color color; // Color on the map (color.h)

    // What we're made of (material names). .size() == made of nothing.
    // MATERIALS WORK IN PROGRESS.
    std::vector<std::string> materials;

    phase_id phase; //e.g. solid, liquid, gas

    unsigned int volume; // Space taken up by this item
    int stack_size;      // How many things make up the above-defined volume (eg. 100 aspirin = 1 volume)
    unsigned int weight; // Weight in grams. Assumes positive weight. No helium, guys!
    bigness_property_aspect bigness_aspect;
    std::map<std::string, int> qualities; //Tool quality indicators
    std::map<std::string, std::string> properties;

    // Explosion that happens when the item is set on fire
    explosion_data explosion_on_fire_data;
    bool explode_in_fire() const
    {
        return explosion_on_fire_data.power >= 0;
    }

    signed int melee_dam; // Bonus for melee damage; may be a penalty
    signed int melee_cut; // Cutting damage in melee
    signed int m_to_hit;  // To-hit bonus for melee combat; -5 to 5 is reasonable

    std::set<std::string> item_tags;
    std::set<std::string> techniques;

    unsigned int light_emission;   // Exactly the same as item_tags LIGHT_*, this is for lightmap.

    const item_category *category; // category pointer or NULL for automatic selection

    std::string snippet_category;

    virtual std::string get_item_type_string() const
    {
        if( container ) {
            return "CONTAINER";
        } else if( armor ) {
            return "ARMOR";
        } else if( book ) {
            return "BOOK";
        } else if( gun.get() != nullptr ) {
            return "GUN";
        }
        return "misc";
    }

    // Returns the name of the item type in the correct language and with respect to its grammatical number,
    // based on quantity (example: item type “anvil”, nname(4) would return “anvils” (as in “4 anvils”).
    virtual std::string nname(unsigned int quantity) const
    {
        return ngettext(name.c_str(), name_plural.c_str(), quantity);
    }

    virtual bool is_food() const
    {
        return false;
    }
    virtual bool is_ammo() const
    {
        return false;
    }
    virtual bool is_gunmod() const
    {
        return false;
    }
    virtual bool is_bionic() const
    {
        return false;
    }
    virtual bool is_tool() const
    {
        return false;
    }
    virtual bool is_software() const
    {
        return false;
    }
    virtual bool is_artifact() const
    {
        return false;
    }
    virtual bool is_var_veh_part() const
    {
        return false;
    }
    virtual bool is_engine() const
    {
        return false;
    }
    virtual bool is_wheel() const
    {
        return false;
    }
    virtual bool count_by_charges() const
    {
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
    bool can_use( std::string iuse_name ) const;
    int invoke( player *p, item *it, bool active, point pos );

    std::vector<use_function> use_methods;// Special effects of use

    itype() : id("null"), price(0), name("none"), name_plural("none"), description(), sym('#'),
        color(c_white), phase(SOLID), volume(0), stack_size(0),
        weight(0), bigness_aspect(BIGNESS_ENGINE_NULL), qualities(),
        melee_dam(0), melee_cut(0), m_to_hit(0), item_tags(), techniques(), light_emission(),
        category(NULL)
    {}

    itype(std::string pid, unsigned int pprice, std::string pname, std::string pname_plural,
          std::string pdes, char psym, nc_color pcolor, std::vector<std::string> pmaterials,
          phase_id pphase, unsigned int pvolume, unsigned int pweight, signed int pmelee_dam,
          signed int pmelee_cut, signed int pm_to_hit) : id(pid), price(pprice), name(pname),
        name_plural(pname_plural), description(pdes), sym(psym), color(pcolor), materials(pmaterials),
        phase(pphase), volume(pvolume), stack_size(0), weight(pweight),
        bigness_aspect(BIGNESS_ENGINE_NULL), qualities(), melee_dam(pmelee_dam),
        melee_cut(pmelee_cut), m_to_hit(pm_to_hit), item_tags(), techniques(), light_emission(),
        category(NULL) { }

    virtual ~itype() {}
};

// Includes food drink and drugs
struct it_comest : public virtual itype {
    signed int quench;     // Many things make you thirstier!
    unsigned int nutr;     // Nutrition imparted
    /**
     * How long it takes to spoil (turns), rotten food is handled differently
     * (chance of bad thinks happen when eating etc).
     * If 0, the food never spoils.
     */
    int spoils;
    unsigned int addict;   // Addictiveness potential
    long charges;  // Defaults # of charges (drugs, loaf of bread? etc)
    std::vector<long> rand_charges;
    signed int stim;
    signed int healthy;
    unsigned int brewtime; // How long it takes for a brew to ferment.
    std::string comesttype; //FOOD, DRINK, MED

    signed int fun;    // How fun its use is

    itype_id default_container; // The container it comes in
    itype_id tool;      // Tool needed to consume (e.g. lighter for cigarettes)

    virtual bool is_food() const
    {
        return true;
    }
    virtual std::string get_item_type_string() const
    {
        return "FOOD";
    }

    virtual bool count_by_charges() const
    {
        if (phase == LIQUID) {
            return true;
        } else {
            return charges > 1 ;
        }
    }

    add_type add; // Effects of addiction

    it_comest(): itype(), quench(0), nutr(0), charges(0), rand_charges(), stim(0), healthy(0),
        brewtime(0), comesttype(), fun(0), default_container(), tool()
    {
    }
};

// v6, v8, wankel, etc.
struct it_var_veh_part: public virtual itype {
    // TODO? geometric mean: nth root of product
    unsigned int min_bigness; //CC's
    unsigned int max_bigness;
    bool engine;

    it_var_veh_part()
        : itype()
        , min_bigness(0)
        , max_bigness(0)
        , engine(false)
    {
    }

    virtual bool is_var_veh_part() const
    {
        return true;
    }
    virtual bool is_wheel() const
    {
        return false;
    }
    virtual bool is_engine() const
    {
        return engine;
    }
    virtual std::string get_item_type_string() const
    {
        return "VEHICLE_PART";
    }
};


struct it_ammo : public virtual itype {
    ammotype type;          // Enum of varieties (e.g. 9mm, shot, etc)
    itype_id casing;        // Casing produced by the ammo, if any
    unsigned int damage;   // Average damage done
    unsigned int pierce;   // Armor piercing; static reduction in armor
    unsigned int range;    // Maximum range
    signed int dispersion; // Dispersion (low is good)
    unsigned int recoil;   // Recoil; modified by strength
    unsigned int count;    // Default charges

    itype_id default_container; // The container it comes in

    std::set<std::string> ammo_effects;

    it_ammo(): itype(), type(), casing(), damage(0), pierce(0), range(0), dispersion(0), recoil(0),
        count(0), default_container(), ammo_effects()
    {
    }

    virtual bool is_ammo() const
    {
        return true;
    }
    // virtual bool count_by_charges() { return id != "gasoline"; }
    virtual bool count_by_charges() const
    {
        return true;
    }
    virtual std::string get_item_type_string() const
    {
        return "AMMO";
    }
};

struct it_gunmod : public virtual itype {
    int dispersion;
    int mod_dispersion;
    int sight_dispersion;
    int aim_speed;
    int damage;
    int loudness;
    int clip;
    int recoil;
    int burst;
    int range;
    int req_skill;
    Skill *skill_used;
    // Rest of the attributes are properly part of a gunmod.
    ammotype newtype;
    std::set<std::string> acceptible_ammo_types;
    bool used_on_pistol;
    bool used_on_shotgun;
    bool used_on_smg;
    bool used_on_rifle;
    bool used_on_bow;
    bool used_on_crossbow;
    bool used_on_launcher;
    std::string location;

    virtual bool is_gunmod() const {
        return true;
    }

    it_gunmod() : itype(), dispersion(0), mod_dispersion(0), sight_dispersion(0),
        aim_speed(0), damage(0), loudness(0), clip(0), recoil(0), burst(0), range(0),
        req_skill(0), skill_used(NULL), newtype(), acceptible_ammo_types(), used_on_pistol(false),
        used_on_shotgun(false), used_on_smg(false), used_on_rifle(false), used_on_bow(false),
        used_on_crossbow(false), used_on_launcher(false), location() {}
};

struct it_tool : public virtual itype {
    ammotype ammo;
    long max_charges;
    long def_charges;
    std::vector<long> rand_charges;
    unsigned char charges_per_use;
    unsigned char turns_per_charge;
    itype_id revert_to;
    itype_id subtype;

    virtual bool is_tool() const
    {
        return true;
    }
    virtual bool is_artifact() const
    {
        return false;
    }
    virtual std::string get_item_type_string() const
    {
        return "TOOL";
    }
    int charges_to_use() const
    {
        return charges_per_use;
    }
    int maximum_charges() const
    {
        return max_charges;
    }
    it_tool() : itype(), ammo(), max_charges(0), def_charges(0), rand_charges(), charges_per_use(0),
        turns_per_charge(0), revert_to(), subtype()
    {
    }
};

struct it_bionic : public virtual itype {
    int difficulty;

    it_bionic() : itype(), difficulty(0)
    {
    }

    virtual bool is_bionic() const
    {
        return true;
    }
    virtual std::string get_item_type_string() const
    {
        return "BIONIC";
    }
};

struct it_software : public virtual itype {
    software_type swtype;
    int power;

    virtual bool is_software() const
    {
        return true;
    }

    it_software(std::string pid, unsigned int pprice, std::string pname,
                std::string pname_plural, std::string pdes, char psym, nc_color pcolor,
                std::vector<std::string> pmaterial, unsigned int pvolume,
                unsigned int pweight, int pmelee_dam, int pmelee_cut, int pm_to_hit,
                software_type pswtype, int ppower)
        : itype(pid, pprice, pname, pname_plural, pdes, psym, pcolor, pmaterial, SOLID,
                pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit)
    {
        swtype = pswtype;
        power = ppower;
    }
};

#endif

