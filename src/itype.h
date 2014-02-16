#ifndef _ITYPE_H_
#define _ITYPE_H_

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
#include "json.h"

#include <string>
#include <vector>
#include <sstream>
#include <set>

// mfb(n) converts a flag to its appropriate position in covers's bitfield
#ifndef mfb
#define mfb(n) static_cast <unsigned long> (1 << (n))
#endif

// for use in category specific inventory lists
enum item_cat
{
  IC_NULL = 0,
  IC_COMESTIBLE,
  IC_AMMO,
  IC_ARMOR,
  IC_GUN,
  IC_BOOK,
  IC_TOOL,
  IC_CONTAINER
};

typedef std::string itype_id;
extern std::vector<std::string> artifact_itype_ids;
extern std::vector<std::string> standard_itype_ids;

// see item_factory.h
class item_category;

struct itype;
extern std::map<std::string, itype*> itypes;

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

enum technique_id {
TEC_NULL,
// Offensive Techniques
TEC_SWEEP,      // Crits may make your enemy fall & miss a turn
TEC_PRECISE,    // Crits are painful and stun
TEC_BRUTAL,     // Crits knock the target back
TEC_GRAB,       // Hit may allow a second unarmed attack attempt
TEC_WIDE,       // Attacks adjacent oppoents
TEC_RAPID,      // Hits faster
TEC_FEINT,      // Misses take less time
TEC_THROW,      // Attacks may throw your opponent
TEC_DISARM,     // Remove an NPC's weapon
TEC_FLAMING,    // Sets victim on fire
// Defensive Techniques
TEC_BLOCK,      // Block attacks, reducing them to 25% damage
TEC_BLOCK_LEGS, // Block attacks, but with your legs
TEC_WBLOCK_1,   // Weapon block, poor chance -- e.g. pole
TEC_WBLOCK_2,   // Weapon block, moderate chance -- weapon made for blocking
TEC_WBLOCK_3,   // Weapon block, good chance -- shield
TEC_COUNTER,    // Counter-attack on a block or dodge
TEC_BREAK,      // Break from a grab
TEC_DEF_THROW,  // Throw an enemy that attacks you
TEC_DEF_DISARM, // Disarm an enemy

NUM_TECHNIQUES
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

struct itype
{
    itype_id id; // ID # that matches its place in master itype list
                 // Used for save files; aligns to itype_id above.
    unsigned int  price; // Its value

    std::string name;        // Proper name
    std::string description; // Flavor text

    char sym;       // Symbol on the map
    nc_color color; // Color on the map (color.h)

    std::string m1; // Main material
    std::string m2; // Secondary material -- "null" if made of just 1 thing

    phase_id phase; //e.g. solid, liquid, gas

    unsigned int volume; // Space taken up by this item
    int stack_size;      // How many things make up the above-defined volume (eg. 100 aspirin = 1 volume)
    unsigned int weight; // Weight in grams. Assumes positive weight. No helium, guys!
    bigness_property_aspect bigness_aspect;
    std::map<std::string, int> qualities; //Tool quality indicators


 mtype*   corpse;

 signed int melee_dam; // Bonus for melee damage; may be a penalty
 signed int melee_cut; // Cutting damage in melee
 signed int m_to_hit;  // To-hit bonus for melee combat; -5 to 5 is reasonable

 std::set<std::string> item_tags;
 std::set<std::string> techniques;
 unsigned int light_emission;   // Exactly the same as item_tags LIGHT_*, this is for lightmap.

 const item_category *category; // category pointer or NULL for automatic selection

 virtual bool is_food()          { return false; }
 virtual bool is_ammo()          { return false; }
 virtual bool is_gun()           { return false; }
 virtual bool is_gunmod()        { return false; }
 virtual bool is_bionic()        { return false; }
 virtual bool is_armor()         { return false; }
 virtual bool is_power_armor()   { return false; }
 virtual bool is_book()          { return false; }
 virtual bool is_tool()          { return false; }
 virtual bool is_container()     { return false; }
 virtual bool is_software()      { return false; }
 virtual bool is_macguffin()     { return false; }
 virtual bool is_stationary()    { return false; }
 virtual bool is_artifact()      { return false; }
 virtual bool is_var_veh_part()  { return false; }
 virtual bool is_engine()        { return false; }
 virtual bool is_wheel()         { return false; }
 virtual bool count_by_charges() { return false; }
 virtual int  charges_to_use()  { return 1; }

 std::string dmg_adj(int dam) { return material_type::find_material(m1)->dmg_adj(dam); }

 use_function use;// Special effects of use

 itype() : id("null"), name("none"), m1("null"), m2("null"), category(0) {
  price = 0;
  sym = '#';
  color = c_white;
  phase = SOLID;
  volume = 0;
  weight = 0;
  bigness_aspect = BIGNESS_ENGINE_NULL;
  corpse = NULL;
  melee_dam = 0;
  melee_cut = 0;
  m_to_hit = 0;
  light_emission = 0;
  use = &iuse::none;
 }

 itype(std::string pid, unsigned int pprice,
       std::string pname, std::string pdes,
       char psym, nc_color pcolor, std::string pm1, std::string pm2, phase_id pphase,
       unsigned int pvolume, unsigned int pweight,
       signed int pmelee_dam, signed int pmelee_cut, signed int pm_to_hit) :
    id(pid), name(pname), description(pdes), m1(pm1), m2(pm2), category(0) {
  price       = pprice;
  sym         = psym;
  color       = pcolor;
  phase       = pphase;
  volume      = pvolume;
  weight      = pweight;
  bigness_aspect = BIGNESS_ENGINE_NULL;
  corpse      = NULL;
  melee_dam   = pmelee_dam;
  melee_cut   = pmelee_cut;
  m_to_hit    = pm_to_hit;
  light_emission = 0;
  use         = &iuse::none;
 }
 virtual ~itype() {}
};

// Includes food drink and drugs
struct it_comest : public virtual itype
{
    signed int quench;     // Many things make you thirstier!
    unsigned int nutr;     // Nutrition imparted
    unsigned int spoils;   // How long it takes to spoil (hours / 600 turns)
    unsigned int addict;   // Addictiveness potential
    unsigned int charges;  // Defaults # of charges (drugs, loaf of bread? etc)
    signed int stim;
    signed int healthy;
    std::string comesttype; //FOOD, DRINK, MED

    signed int fun;    // How fun its use is

    itype_id container; // The container it comes in
    itype_id tool;      // Tool needed to consume (e.g. lighter for cigarettes)

    virtual bool is_food() { return true; }

    virtual bool count_by_charges()
    {
        if (phase == LIQUID) {
            return true;
        } else {
            return charges > 1 ;
        }
    }

    add_type add; // Effects of addiction

    it_comest(std::string pid, unsigned int pprice,
    std::string pname, std::string pdes,
    char psym, nc_color pcolor, std::string pm1, phase_id pphase,
    unsigned int pvolume, unsigned int pweight,
    signed int pmelee_dam, signed int pmelee_cut,
    signed int pm_to_hit,

    signed int pquench, unsigned int pnutr, signed int pspoils,
    signed int pstim, signed int phealthy, unsigned int paddict,
    unsigned int pcharges, signed int pfun, itype_id pcontainer,
    itype_id ptool, int (iuse::*puse)(player *, item *, bool),
    add_type padd, std::string pcomesttype)
    : itype(pid, pprice, pname, pdes, psym, pcolor, pm1, "null", pphase,
    pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit), comesttype(pcomesttype), container(pcontainer), tool(ptool)
    {
        quench     = pquench;
        nutr       = pnutr;
        spoils     = pspoils;
        stim       = pstim;
        healthy    = phealthy;
        addict     = paddict;
        charges    = pcharges;
        fun        = pfun;
        use        = puse;
        add        = padd;
    }

    it_comest() : itype()
    {
        quench = 0;
        nutr = 0;
        spoils = 0;
        stim = 0;
        healthy = 0;
        addict = 0;
        charges = 0;
        fun = 0;
        add = ADD_NULL;
    };
};

// v6, v8, wankel, etc.
struct it_var_veh_part: public virtual itype
{
 // TODO? geometric mean: nth root of product
 unsigned int min_bigness; //CC's
 unsigned int max_bigness;
 bool engine;

 it_var_veh_part() { }
 it_var_veh_part(std::string pid, unsigned int pprice,
        std::string pname, std::string pdes,
        char psym, nc_color pcolor, std::string pm1, std::string pm2,
        unsigned int pvolume, unsigned int pweight,
        signed int pmelee_dam, signed int pmelee_cut, signed int pm_to_hit,

        unsigned int big_min,
        unsigned int big_max,
        bigness_property_aspect big_aspect, bool pengine)
:itype(pid, pprice, pname, pdes, psym, pcolor, pm1, pm2, SOLID,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit) {
  min_bigness = big_min;
  max_bigness = big_max;
  bigness_aspect = big_aspect;
  engine = pengine;
 }
 virtual bool is_var_veh_part(){return true;}
 virtual bool is_wheel()          { return false; }
 virtual bool is_engine() { return engine; }
};


struct it_ammo : public virtual itype
{
 ammotype type;          // Enum of varieties (e.g. 9mm, shot, etc)
 itype_id casing;        // Casing produced by the ammo, if any
 unsigned int damage;   // Average damage done
 unsigned int pierce;   // Armor piercing; static reduction in armor
 unsigned int range;    // Maximum range
 signed int dispersion; // Dispersion (low is good)
 unsigned int recoil;   // Recoil; modified by strength
 unsigned int count;    // Default charges

 std::set<std::string> ammo_effects;

 virtual bool is_ammo() { return true; }
// virtual bool count_by_charges() { return id != "gasoline"; }
 virtual bool count_by_charges() { return true; }

 it_ammo() : itype()
 {
     type = "NULL";
     casing = "NULL";
     damage = 0;
     pierce = 0;
     range = 0;
     dispersion = 0;
     recoil = 0;
     count = 0;
 }

 it_ammo(std::string pid, unsigned int pprice,
        std::string pname, std::string pdes,
        char psym, nc_color pcolor, std::string pm1, phase_id pphase,
        unsigned int pvolume, unsigned int pweight,
        signed int pmelee_dam, signed int pmelee_cut, signed int pm_to_hit,
        std::set<std::string> effects,
        ammotype ptype, itype_id pcasing,
        unsigned int pdamage, unsigned int ppierce,
        signed int pdispersion, unsigned int precoil, unsigned int prange,
        unsigned int pcount)
:itype(pid, pprice, pname, pdes, psym, pcolor, pm1, "null", pphase,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit) {
  type = ptype;
  casing = pcasing;
  damage = pdamage;
  pierce = ppierce;
  range = prange;
  dispersion = pdispersion;
  recoil = precoil;
  count = pcount;
  ammo_effects = effects;
 }
};

struct it_gun : public virtual itype
{
 ammotype ammo;
 Skill *skill_used;
 signed int dmg_bonus;
 signed int pierce;
 signed int range;
 signed int dispersion;
 signed int recoil;
 signed int durability;
 unsigned int burst;
 int clip;
 int reload_time;

 std::set<std::string> ammo_effects;
 std::map<std::string, int> valid_mod_locations;

 virtual bool is_gun() { return true; }

 it_gun(std::string pid, unsigned int pprice,
        std::string pname, std::string pdes,
        char psym, nc_color pcolor, std::string pm1, std::string pm2,
        unsigned int pvolume, unsigned int pweight,
        signed int pmelee_dam, signed int pmelee_cut, signed int pm_to_hit,
        signed int ppierce,
        std::set<std::string> flags,
        std::set<std::string> effects,
        const char *pskill_used, ammotype pammo,
        signed int pdmg_bonus, signed int prange,
        signed int pdispersion, signed int precoil, unsigned int pdurability,
        unsigned int pburst, int pclip, int preload_time)
:itype(pid, pprice, pname, pdes, psym, pcolor, pm1, pm2, SOLID,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit) {
  skill_used = pskill_used?Skill::skill(pskill_used):NULL;
  ammo = pammo;
  dmg_bonus = pdmg_bonus;
  pierce = ppierce;
  range = prange;
  dispersion = pdispersion;
  recoil = precoil;
  durability = pdurability;
  burst = pburst;
  clip = pclip;
  reload_time = preload_time;
  ammo_effects = effects;
  item_tags = flags;
 }

 it_gun() :itype() {
  ammo = "";
  skill_used = NULL;
  dmg_bonus = 0;
  pierce = 0;
  range = 0;
  dispersion = 0;
  recoil = 0;
  durability = 0;
  burst = 0;
  clip = 0;
  reload_time = 0;
 };
};

struct it_gunmod : public virtual itype
{
 signed int dispersion, damage, loudness, clip, recoil, burst;
 ammotype newtype;
 std::set<std::string> acceptible_ammo_types;
 bool used_on_pistol;
 bool used_on_shotgun;
 bool used_on_smg;
 bool used_on_rifle;
 bool used_on_bow;
 bool used_on_crossbow;
 bool used_on_launcher;
 Skill *skill_used;
 std::string location;

 virtual bool is_gunmod() { return true; }

 it_gunmod(std::string pid, unsigned int pprice,
           std::string pname, std::string pdes,
           char psym, nc_color pcolor, std::string pm1, std::string pm2,
           unsigned int pvolume, unsigned int pweight,
           signed int pmelee_dam, signed int pmelee_cut,
           signed int pm_to_hit,

           signed int pdispersion, signed int pdamage, signed int ploudness,
           signed int pclip, signed int precoil, signed int pburst,
           ammotype pnewtype, std::set<std::string> a_a_t, bool pistol,
           bool shotgun, bool smg, bool rifle, char *pskill_used)

 :itype(pid, pprice, pname, pdes, psym, pcolor, pm1, pm2, SOLID,
        pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit) {

  dispersion = pdispersion;
  damage = pdamage;
  loudness = ploudness;
  clip = pclip;
  recoil = precoil;
  burst = pburst;
  newtype = pnewtype;
  acceptible_ammo_types = a_a_t;
  used_on_pistol = pistol;
  used_on_shotgun = shotgun;
  used_on_smg = smg;
  used_on_rifle = rifle;
  skill_used = pskill_used?Skill::skill(pskill_used):NULL;
 }

 it_gunmod() :itype() {
  dispersion = 0;
  damage = 0;
  loudness = 0;
  clip = 0;
  recoil = 0;
  burst = 0;
  newtype = "";
  used_on_pistol = false;
  used_on_shotgun = false;
  used_on_smg = false;
  used_on_rifle = false;
  location = "";
  skill_used = NULL;
 };
};

struct it_armor : public virtual itype
{
 unsigned char covers; // Bitfield of enum body_part
 signed char encumber;
 unsigned char coverage;
 unsigned char thickness;
 unsigned char env_resist; // Resistance to environmental effects
 signed char warmth;
 unsigned char storage;

 bool power_armor;

 virtual bool is_armor() { return true; }
 virtual bool is_power_armor() { return power_armor; }
 virtual bool is_artifact() { return false; }
 std::string bash_dmg_verb() { return m2 == "null" || !one_in(3) ?
         material_type::find_material(m1)->bash_dmg_verb() :
         material_type::find_material(m2)->bash_dmg_verb();
 }
 std::string cut_dmg_verb() { return m2 == "null" || !one_in(3) ?
         material_type::find_material(m1)->cut_dmg_verb() :
         material_type::find_material(m2)->cut_dmg_verb();
 }

 it_armor() : itype()
 {
  covers = 0;
  encumber = 0;
  coverage = 0;
  thickness = 0;
  env_resist = 0;
  warmth = 0;
  storage = 0;
  power_armor = false;
 }

 it_armor(itype_id pid, unsigned int pprice,
          std::string pname, std::string pdes,
          char psym, nc_color pcolor, std::string pm1, std::string pm2,
          unsigned int pvolume, unsigned int pweight,
          signed int pmelee_dam, signed int pmelee_cut, signed int pm_to_hit,

          unsigned char pcovers, signed char pencumber,
          unsigned char pcoverage, unsigned char pthickness,
          unsigned char penv_resist, signed char pwarmth,
          unsigned char pstorage, bool ppower_armor = false)
:itype(pid, pprice, pname, pdes, psym, pcolor, pm1, pm2, SOLID,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit) {
  covers = pcovers;
  encumber = pencumber;
  coverage = pcoverage;
  thickness = pthickness;
  env_resist = penv_resist;
  warmth = pwarmth;
  storage = pstorage;
  power_armor = ppower_armor;
 }
};

struct recipe;

struct it_book : public virtual itype
{
 Skill *type;         // Which skill it upgrades
 unsigned char level; // The value it takes the skill to
 unsigned char req;   // The skill level required to understand it
 signed char fun;     // How fun reading this is
 unsigned char intel; // Intelligence required to read, at all
 unsigned int time;  // How long, in 10-turns (aka minutes), it takes to read
                      // "To read" means getting 1 skill point, not all of em
 int chapters; //Fun books have chapters; after all are read, the book is less fun
 std::map<recipe*, int> recipes; //what recipes can be learned from this book
 virtual bool is_book() { return true; }
 it_book() {}
 it_book(std::string pid, unsigned int pprice,
         std::string pname, std::string pdes,
         char psym, nc_color pcolor, std::string pm1, std::string pm2,
         unsigned int pvolume, unsigned int pweight,
         signed int pmelee_dam, signed int pmelee_cut, signed int pm_to_hit,

         const char *ptype, unsigned char plevel, unsigned char preq,
         signed char pfun, unsigned char pintel, unsigned int ptime)
:itype(pid, pprice, pname, pdes, psym, pcolor, pm1, pm2, SOLID,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit) {
  type = ptype?Skill::skill(ptype):NULL;
  level = plevel;
  req = preq;
  fun = pfun;
  intel = pintel;
  time = ptime;
 }
};

struct it_container : public virtual itype
{
 unsigned int contains; // Internal volume
 virtual bool is_container() { return true; }
 it_container() : contains(0) {};
};

struct it_tool : public virtual itype
{
 ammotype ammo;
 unsigned int max_charges;
 unsigned int def_charges;
 unsigned char charges_per_use;
 unsigned char turns_per_charge;
 itype_id revert_to;

 virtual bool is_tool()          { return true; }
 virtual bool is_artifact()      { return false; }
 int charges_to_use()   { return charges_per_use; }

 it_tool() :itype()
 {
  ammo = "NULL";
  max_charges = 0;
  def_charges = 0;
  charges_per_use = 0;
  turns_per_charge = 0;
  revert_to = "null";
  use = &iuse::none;
 }

 it_tool(std::string pid, unsigned int pprice,
         std::string pname, std::string pdes,
         char psym, nc_color pcolor, std::string pm1, std::string pm2, phase_id pphase,
         unsigned int pvolume, unsigned int pweight,
         signed int pmelee_dam, signed int pmelee_cut, signed int pm_to_hit,

         unsigned int pmax_charges, unsigned int pdef_charges,
         unsigned char pcharges_per_use, unsigned char pturns_per_charge,
         ammotype pammo, itype_id prevert_to,
         int (iuse::*puse)(player *, item *, bool))
:itype(pid, pprice, pname, pdes, psym, pcolor, pm1, pm2, pphase,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit) {
  max_charges = pmax_charges;
  def_charges = pdef_charges;
  ammo = pammo;
  charges_per_use = pcharges_per_use;
  turns_per_charge = pturns_per_charge;
  revert_to = prevert_to;
  use = puse;
 }
};

struct it_bionic : public virtual itype
{
 int difficulty;

 virtual bool is_bionic()    { return true; }
 it_bionic() { }
 it_bionic(std::string pid, unsigned int pprice,
           std::string pname, std::string pdes,
           char psym, nc_color pcolor, std::string pm1, std::string pm2,
           unsigned int pvolume, unsigned int pweight,
           signed char pmelee_dam, signed char pmelee_cut,
           signed char pm_to_hit,
           int pdifficulty)
 :itype(pid, pprice, pname, pdes, psym, pcolor, pm1, pm2, SOLID,
        pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit) {
   difficulty = pdifficulty;
 }
};

struct it_macguffin : public virtual itype
{
 bool readable; // If true, activated with 'R'

 virtual bool is_macguffin() { return true; }

 it_macguffin(std::string pid, unsigned int pprice,
              std::string pname, std::string pdes,
              char psym, nc_color pcolor, std::string pm1, std::string pm2,
              unsigned int pvolume, unsigned int pweight,
              signed int pmelee_dam, signed int pmelee_cut,
              signed int pm_to_hit,

              bool preadable,
              int (iuse::*puse)(player *, item *, bool))
:itype(pid, pprice, pname, pdes, psym, pcolor, pm1, pm2, SOLID,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit) {
  readable = preadable;
  use = puse;
 }
};

struct it_software : public virtual itype
{
 software_type swtype;
 int power;

 virtual bool is_software()      { return true; }

 it_software(std::string pid, unsigned int pprice,
             std::string pname, std::string pdes,
             char psym, nc_color pcolor, std::string pm1, std::string pm2,
             unsigned int pvolume, unsigned int pweight,
             signed int pmelee_dam, signed int pmelee_cut,
             signed int pm_to_hit,

             software_type pswtype, int ppower)
:itype(pid, pprice, pname, pdes, psym, pcolor, pm1, pm2, SOLID,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit) {
  swtype = pswtype;
  power = ppower;
 }
};

struct it_stationary : public virtual itype
{
 virtual bool is_stationary()         { return true; }

 std::string category;

 it_stationary(std::string pid, unsigned int pprice,
          std::string pname, std::string pdes,
          char psym, nc_color pcolor, std::string pm1, std::string pm2,
          unsigned char pvolume, unsigned char pweight,
          signed int pmelee_dam, signed int pmelee_cut,
          signed int pm_to_hit,
          std::string pcategory)

:itype(pid, pprice, pname, pdes, psym, pcolor, pm1, pm2, SOLID,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit)
 {
     category = pcategory;
 }
};

#endif
