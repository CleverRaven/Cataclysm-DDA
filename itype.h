#ifndef _ITYPE_H_
#define _ITYPE_H_

#include "color.h"
#include "enums.h"
#include "iuse.h"
#include "pldata.h"
#include "bodypart.h"
#include "skill.h"
#include "bionics.h"
#include "artifact.h"
#include "picojson.h"
#include "rng.h"
#include "material.h"
#include <string>
#include <vector>
#include <sstream>
#include <set>

// mfb(n) converts a flag to its appropriate position in covers's bitfield
#ifndef mfb
#define mfb(n) long(1 << (n))
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
extern std::vector<std::string> unreal_itype_ids;
extern std::vector<std::string> martial_arts_itype_ids;
extern std::vector<std::string> artifact_itype_ids;
extern std::vector<std::string> standard_itype_ids;
extern std::vector<std::string> pseudo_itype_ids;

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
TEC_SWEEP,	// Crits may make your enemy fall & miss a turn
TEC_PRECISE,	// Crits are painful and stun
TEC_BRUTAL,	// Crits knock the target back
TEC_GRAB,	// Hit may allow a second unarmed attack attempt
TEC_WIDE,	// Attacks adjacent oppoents
TEC_RAPID,	// Hits faster
TEC_FEINT,	// Misses take less time
TEC_THROW,	// Attacks may throw your opponent
TEC_DISARM,	// Remove an NPC's weapon
TEC_FLAMING,    // Sets victim on fire
// Defensive Techniques
TEC_BLOCK,	// Block attacks, reducing them to 25% damage
TEC_BLOCK_LEGS, // Block attacks, but with your legs
TEC_WBLOCK_1,	// Weapon block, poor chance -- e.g. pole
TEC_WBLOCK_2,	// Weapon block, moderate chance -- weapon made for blocking
TEC_WBLOCK_3,	// Weapon block, good chance -- shield
TEC_COUNTER,	// Counter-attack on a block or dodge
TEC_BREAK,	// Break from a grab
TEC_DEF_THROW,	// Throw an enemy that attacks you
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

struct style_move
{
 std::string name;
 std::string verb_you;
 std::string verb_npc;
 technique_id tech;
 int level;

 style_move(std::string N, std::string V1, std::string V2, technique_id T, int L) :
  name (N),verb_you (V1),verb_npc (V2), tech (T), level (L) { };

 style_move()
 {
  name = verb_you = verb_npc = "";
  tech = TEC_NULL;
  level = 0;
 }
};

// Returns the name of a category of ammo (e.g. "shot")
std::string ammo_name(ammotype t);
// Returns the default ammo for a category of ammo (e.g. ""00_shot"")
itype_id default_ammo(ammotype guntype);

struct itype
{
 itype_id id;		// ID # that matches its place in master itype list
 			// Used for save files; aligns to itype_id above.
 unsigned char rarity;	// How often it's found
 unsigned int  price;	// Its value

 std::string name;	// Proper name
 std::string description;// Flavor text

 char sym;		// Symbol on the map
 nc_color color;	// Color on the map (color.h)

 std::string m1;		// Main material
 std::string m2;		// Secondary material -- "null" if made of just 1 thing

 phase_id phase;      //e.g. solid, liquid, gas

 unsigned int volume;	// Space taken up by this item
 unsigned int weight;	// Weight in grams. Assumes positive weight. No helium, guys!
 bigness_property_aspect bigness_aspect;

 signed char melee_dam;	// Bonus for melee damage; may be a penalty
 signed char melee_cut;	// Cutting damage in melee
 signed char m_to_hit;	// To-hit bonus for melee combat; -5 to 5 is reasonable

 std::set<std::string> item_tags;
 unsigned techniques : NUM_TECHNIQUES;
 unsigned int light_emission;   // Exactly the same as item_tags LIGHT_*, this is for lightmap.

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
 virtual bool is_style()         { return false; }
 virtual bool is_stationary()    { return false; }
 virtual bool is_artifact()      { return false; }
 virtual bool is_var_veh_part()  { return false; }
 virtual bool is_engine()         { return false; }
 virtual bool is_wheel()          { return false; }
 virtual bool count_by_charges() { return false; }
 virtual picojson::value save_data() { return picojson::value(); }

 std::string dmg_adj(int dam) { return material_type::find_material(m1)->dmg_adj(dam); }

 void (iuse::*use)(game *, player *, item *, bool);// Special effects of use

 itype() {
  id = "null";
  rarity = 0;
  name  = "none";
  sym = '#';
  color = c_white;
  m1 = "null";
  m2 = "null";
  phase = SOLID;
  volume = 0;
  weight = 0;
  melee_dam = 0;
  m_to_hit = 0;
  techniques = 0;
  light_emission = 0;
  use = &iuse::none;
 }

 itype(std::string pid, unsigned char prarity, unsigned int pprice,
       std::string pname, std::string pdes,
       char psym, nc_color pcolor, std::string pm1, std::string pm2, phase_id pphase,
       unsigned short pvolume, unsigned int pweight,
       signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
       unsigned ptechniques = 0) {
  id          = pid;
  rarity      = prarity;
  price       = pprice;
  name        = pname;
  description = pdes;
  sym         = psym;
  color       = pcolor;
  m1          = pm1;
  m2          = pm2;
  phase       = pphase;
  volume      = pvolume;
  weight      = pweight;
  melee_dam   = pmelee_dam;
  melee_cut   = pmelee_cut;
  m_to_hit    = pm_to_hit;
  techniques  = ptechniques;
  light_emission = 0;
  use         = &iuse::none;
 }
};

// Includes food drink and drugs
struct it_comest : public itype
{
    signed char quench;	// Many things make you thirstier!
    unsigned char nutr;	// Nutrition imparted
    unsigned char spoils;	// How long it takes to spoil (hours / 600 turns)
    unsigned char addict;	// Addictiveness potential
    unsigned char charges;	// Defaults # of charges (drugs, loaf of bread? etc)
    signed char stim;
    signed char healthy;
    std::string comesttype; //FOOD, DRINK, MED

    signed char fun;	// How fun its use is

    itype_id container;	// The container it comes in
    itype_id tool;		// Tool needed to consume (e.g. lighter for cigarettes)

    virtual bool is_food() { return true; }
    // virtual bool count_by_charges() { return charges >= 1 ; }

    virtual bool count_by_charges()
    {
        if (phase == LIQUID) {
            return true;
        } else {
            return charges > 1 ;
        }
    }

    add_type add;				// Effects of addiction

    it_comest(std::string pid, unsigned char prarity, unsigned int pprice,
    std::string pname, std::string pdes,
    char psym, nc_color pcolor, std::string pm1, phase_id pphase,
    unsigned short pvolume, unsigned int pweight,
    signed char pmelee_dam, signed char pmelee_cut,
    signed char pm_to_hit,

    signed char pquench, unsigned char pnutr, signed char pspoils,
    signed char pstim, signed char phealthy, unsigned char paddict,
    unsigned char pcharges, signed char pfun, itype_id pcontainer,
    itype_id ptool, void (iuse::*puse)(game *, player *, item *, bool),
    add_type padd, std::string pcomesttype)
    :itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, "null", pphase,
    pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit)
    {
        quench     = pquench;
        nutr       = pnutr;
        spoils     = pspoils;
        stim       = pstim;
        healthy    = phealthy;
        addict     = paddict;
        charges    = pcharges;
        fun        = pfun;
        container  = pcontainer;
        tool       = ptool;
        use        = puse;
        add        = padd;
        comesttype = pcomesttype;
    }

    it_comest() :itype() { };
};

// v6, v8, wankel, etc.
struct it_var_veh_part: public itype
{
 // TODO? geometric mean: nth root of product
 unsigned int min_bigness; //CC's
 unsigned int max_bigness;

 it_var_veh_part(std::string pid, unsigned char prarity, unsigned int pprice,
        std::string pname, std::string pdes,
        char psym, nc_color pcolor, std::string pm1, std::string pm2,
        unsigned short pvolume, unsigned int pweight,
        signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
        unsigned effects,

        unsigned int big_min,
        unsigned int big_max,
        bigness_property_aspect big_aspect)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2, SOLID,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit) {
  min_bigness = big_min;
  max_bigness = big_max;
  bigness_aspect = big_aspect;
 }
 virtual bool is_var_veh_part(){return true;}
 virtual bool is_wheel()          { return false; }
 virtual bool is_engine() {
  //FIX ME OH FUCKING GOD NOT EVERYTING SHOULD BE AN ENGINE
  // TODO: glyphgryph
  return true;
 }
};


struct it_ammo : public itype
{
 ammotype type;		// Enum of varieties (e.g. 9mm, shot, etc)
 unsigned char damage;	// Average damage done
 unsigned char pierce;	// Armor piercing; static reduction in armor
 unsigned char range;	// Maximum range
 signed char dispersion;// Dispersion (low is good)
 unsigned char recoil;	// Recoil; modified by strength
 unsigned char count;	// Default charges

 std::set<std::string> ammo_effects;

 virtual bool is_ammo() { return true; }
// virtual bool count_by_charges() { return id != "gasoline"; }
 virtual bool count_by_charges() { return true; }

 it_ammo() : itype()
 {
     type = "NULL";
     damage = 0;
     pierce = 0;
     range = 0;
     dispersion = 0;
     recoil = 0;
     count = 0;
 }

 it_ammo(std::string pid, unsigned char prarity, unsigned int pprice,
        std::string pname, std::string pdes,
        char psym, nc_color pcolor, std::string pm1, phase_id pphase,
        unsigned short pvolume, unsigned int pweight,
        signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
         std::set<std::string> effects,

        ammotype ptype, unsigned char pdamage, unsigned char ppierce,
	signed char pdispersion, unsigned char precoil, unsigned char prange,
        unsigned char pcount)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, "null", pphase,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, 0) {
  type = ptype;
  damage = pdamage;
  pierce = ppierce;
  range = prange;
  dispersion = pdispersion;
  recoil = precoil;
  count = pcount;
  ammo_effects = effects;
 }
};

struct it_gun : public itype
{
 ammotype ammo;
 Skill *skill_used;
 signed char dmg_bonus;
 signed char range;
 signed char dispersion;
 signed char recoil;
 signed char durability;
 unsigned char burst;
 int clip;
 int reload_time;

 virtual bool is_gun() { return true; }

 it_gun(std::string pid, unsigned char prarity, unsigned int pprice,
        std::string pname, std::string pdes,
        char psym, nc_color pcolor, std::string pm1, std::string pm2,
        unsigned short pvolume, unsigned int pweight,
        signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,

	const char *pskill_used, ammotype pammo, signed char pdmg_bonus, signed char prange,
	signed char pdispersion, signed char precoil, unsigned char pdurability,
        unsigned char pburst, int pclip, int preload_time)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2, SOLID,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit) {
  skill_used = pskill_used?Skill::skill(pskill_used):NULL;
  ammo = pammo;
  dmg_bonus = pdmg_bonus;
  range = prange;
  dispersion = pdispersion;
  recoil = precoil;
  durability = pdurability;
  burst = pburst;
  clip = pclip;
  reload_time = preload_time;
 }

 it_gun() :itype() { };
};

struct it_gunmod : public itype
{
 signed char dispersion, damage, loudness, clip, recoil, burst;
 ammotype newtype;
 std::set<std::string> acceptible_ammo_types;
 bool used_on_pistol;
 bool used_on_shotgun;
 bool used_on_smg;
 bool used_on_rifle;

 virtual bool is_gunmod() { return true; }

 it_gunmod(std::string pid, unsigned char prarity, unsigned int pprice,
           std::string pname, std::string pdes,
           char psym, nc_color pcolor, std::string pm1, std::string pm2,
           unsigned short pvolume, unsigned int pweight,
           signed char pmelee_dam, signed char pmelee_cut,
           signed char pm_to_hit,

           signed char pdispersion, signed char pdamage, signed char ploudness,
           signed char pclip, signed char precoil, signed char pburst,
           ammotype pnewtype, std::set<std::string> a_a_t, bool pistol,
           bool shotgun, bool smg, bool rifle)

 :itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2, SOLID,
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
 }

 it_gunmod() :itype() { };
};

struct it_armor : public itype
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
 virtual picojson::value save_data() { return picojson::value(); }
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

 it_armor(itype_id pid, unsigned char prarity, unsigned int pprice,
          std::string pname, std::string pdes,
          char psym, nc_color pcolor, std::string pm1, std::string pm2,
          unsigned short pvolume, unsigned int pweight,
          signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,

          unsigned char pcovers, signed char pencumber,
          unsigned char pcoverage, unsigned char pthickness,
          unsigned char penv_resist, signed char pwarmth,
          unsigned char pstorage, bool ppower_armor = false)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2, SOLID,
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

struct it_book : public itype
{
 Skill *type;		// Which skill it upgrades
 unsigned char level;	// The value it takes the skill to
 unsigned char req;	// The skill level required to understand it
 signed char fun;	// How fun reading this is
 unsigned char intel;	// Intelligence required to read, at all
 unsigned char time;	// How long, in 10-turns (aka minutes), it takes to read
			// "To read" means getting 1 skill point, not all of em
 std::map<recipe*, int> recipes; //what recipes can be learned from this book
 virtual bool is_book() { return true; }
 it_book() {}
 it_book(std::string pid, unsigned char prarity, unsigned int pprice,
         std::string pname, std::string pdes,
         char psym, nc_color pcolor, std::string pm1, std::string pm2,
         unsigned short pvolume, unsigned int pweight,
         signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,

	 const char *ptype, unsigned char plevel, unsigned char preq,
	 signed char pfun, unsigned char pintel, unsigned char ptime)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2, SOLID,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit) {
  type = ptype?Skill::skill(ptype):NULL;
  level = plevel;
  req = preq;
  fun = pfun;
  intel = pintel;
  time = ptime;
 }
};

struct it_container : public itype
{
 unsigned char contains;	// Internal volume
 virtual bool is_container() { return true; }
 it_container() {};
};

struct it_tool : public itype
{
 ammotype ammo;
 unsigned int max_charges;
 unsigned int def_charges;
 unsigned char charges_per_use;
 unsigned char turns_per_charge;
 itype_id revert_to;

 virtual bool is_tool()          { return true; }
 virtual bool is_artifact()      { return false; }
 virtual picojson::value save_data() { return picojson::value(); }

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

 it_tool(std::string pid, unsigned char prarity, unsigned int pprice,
         std::string pname, std::string pdes,
         char psym, nc_color pcolor, std::string pm1, std::string pm2, phase_id pphase,
         unsigned short pvolume, unsigned int pweight,
         signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,

         unsigned int pmax_charges, unsigned int pdef_charges,
         unsigned char pcharges_per_use, unsigned char pturns_per_charge,
         ammotype pammo, itype_id prevert_to,
	 void (iuse::*puse)(game *, player *, item *, bool))
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2, pphase,
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

struct it_bionic : public itype
{
 std::vector<bionic_id> options;
 int difficulty;

 virtual bool is_bionic()    { return true; }

 it_bionic(std::string pid, unsigned char prarity, unsigned int pprice,
           std::string pname, std::string pdes,
           char psym, nc_color pcolor, std::string pm1, std::string pm2,
           unsigned short pvolume, unsigned int pweight,
           signed char pmelee_dam, signed char pmelee_cut,
           signed char pm_to_hit,

           int pdifficulty)
 :itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2, SOLID,
        pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit) {
   difficulty = pdifficulty;
   options.push_back(id);
 }
};

struct it_macguffin : public itype
{
 bool readable; // If true, activated with 'R'

 virtual bool is_macguffin() { return true; }

 it_macguffin(std::string pid, unsigned char prarity, unsigned int pprice,
              std::string pname, std::string pdes,
              char psym, nc_color pcolor, std::string pm1, std::string pm2,
              unsigned short pvolume, unsigned int pweight,
              signed char pmelee_dam, signed char pmelee_cut,
              signed char pm_to_hit,

              bool preadable,
              void (iuse::*puse)(game *, player *, item *, bool))
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2, SOLID,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit) {
  readable = preadable;
  use = puse;
 }
};

struct it_software : public itype
{
 software_type swtype;
 int power;

 virtual bool is_software()      { return true; }

 it_software(std::string pid, unsigned char prarity, unsigned int pprice,
             std::string pname, std::string pdes,
             char psym, nc_color pcolor, std::string pm1, std::string pm2,
             unsigned short pvolume, unsigned int pweight,
             signed char pmelee_dam, signed char pmelee_cut,
             signed char pm_to_hit,

             software_type pswtype, int ppower)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2, SOLID,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit) {
  swtype = pswtype;
  power = ppower;
 }
};

struct it_style : public itype
{
 virtual bool is_style()         { return true; }

 std::vector<style_move> moves;

 it_style(std::string pid, unsigned char prarity, unsigned int pprice,
          std::string pname, std::string pdes,
          char psym, nc_color pcolor, std::string pm1, std::string pm2,
          unsigned char pvolume, unsigned char pweight,
          signed char pmelee_dam, signed char pmelee_cut,
          signed char pm_to_hit)

:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2, SOLID,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit) { }
};

struct it_stationary : public itype
{
 virtual bool is_stationary()         { return true; }

 std::string category;

 it_stationary(std::string pid, unsigned char prarity, unsigned int pprice,
          std::string pname, std::string pdes,
          char psym, nc_color pcolor, std::string pm1, std::string pm2,
          unsigned char pvolume, unsigned char pweight,
          signed char pmelee_dam, signed char pmelee_cut,
          signed char pm_to_hit,
          std::string pcategory)

:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2, SOLID,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit)
 {
     category = pcategory;
 }
};

struct it_artifact_tool : public it_tool
{
 art_charge charge_type;
 std::vector<art_effect_passive> effects_wielded;
 std::vector<art_effect_active>  effects_activated;
 std::vector<art_effect_passive> effects_carried;

 virtual bool is_artifact()  { return true; }
 virtual picojson::value save_data()
 {
     std::map<std::string, picojson::value> data;

     data[std::string("type")] = picojson::value("artifact_tool");

     // generic data
     data[std::string("id")] = picojson::value(id);
     data[std::string("price")] = picojson::value(price);
     data[std::string("name")] = picojson::value(name);
     data[std::string("description")] = picojson::value(description);
     data[std::string("sym")] = picojson::value(sym);
     data[std::string("color")] = picojson::value(color_to_int(color));
     data[std::string("m1")] = picojson::value(m1);
     data[std::string("m2")] = picojson::value(m2);
     data[std::string("volume")] = picojson::value(volume);
     data[std::string("weight")] = picojson::value(weight);
     data[std::string("id")] = picojson::value(id);
     data[std::string("melee_dam")] = picojson::value(melee_dam);
     data[std::string("melee_cut")] = picojson::value(melee_cut);
     data[std::string("m_to_hit")] = picojson::value(m_to_hit);

     std::vector<picojson::value> tags_json;
     for(std::set<std::string>::iterator it = item_tags.begin();
         it != item_tags.end(); ++it)
     {
         tags_json.push_back(picojson::value(*it));
     }
     data[std::string("item_flags")] = picojson::value(tags_json);

     data[std::string("techniques")] = picojson::value(techniques);

     // tool data
     data[std::string("ammo")] = picojson::value(ammo);
     data[std::string("max_charges")] = picojson::value(max_charges);
     data[std::string("def_charges")] = picojson::value(def_charges);
     data[std::string("charges_per_use")] = picojson::value(charges_per_use);
     data[std::string("turns_per_charge")] = picojson::value(turns_per_charge);
     data[std::string("revert_to")] = picojson::value(revert_to);

     // artifact data
     data[std::string("charge_type")] = picojson::value(charge_type);

     std::vector<picojson::value> effects_wielded_json;
     for(std::vector<art_effect_passive>::iterator it = effects_wielded.begin();
         it != effects_wielded.end(); ++it)
     {
         effects_wielded_json.push_back(picojson::value(*it));
     }
     data[std::string("effects_wielded")] =
         picojson::value(effects_wielded_json);

     std::vector<picojson::value> effects_activated_json;
     for(std::vector<art_effect_active>::iterator it =
             effects_activated.begin();
         it != effects_activated.end(); ++it)
     {
         effects_activated_json.push_back(picojson::value(*it));
     }
     data[std::string("effects_activated")] =
         picojson::value(effects_activated_json);

     std::vector<picojson::value> effects_carried_json;
     for(std::vector<art_effect_passive>::iterator it = effects_carried.begin();
         it != effects_carried.end(); ++it)
     {
         effects_carried_json.push_back(picojson::value(*it));
     }
     data[std::string("effects_carried")] =
         picojson::value(effects_carried_json);

     return picojson::value(data);
 }

 it_artifact_tool() :it_tool(){
  ammo = "NULL";
  price = 0;
  def_charges = 0;
  charges_per_use = 1;
  turns_per_charge = 0;
  revert_to = "null";
  use = &iuse::artifact;
 };

 it_artifact_tool(std::string pid, unsigned int pprice, std::string pname,
                  std::string pdes, char psym, nc_color pcolor, std::string pm1,
                  std::string pm2, unsigned short pvolume, unsigned int pweight,
                  signed char pmelee_dam, signed char pmelee_cut,
                  signed char pm_to_hit, std::set<std::string> pitem_tags,

                  unsigned int pmax_charges, unsigned int pdef_charges,
                  unsigned char pcharges_per_use,
                  unsigned char pturns_per_charge,
                  ammotype pammo, itype_id prevert_to)

:it_tool(pid, 0, pprice, pname, pdes, psym, pcolor, pm1, pm2, SOLID,
         pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit,
	 pmax_charges, pdef_charges, pcharges_per_use, pturns_per_charge,
	 pammo, prevert_to, &iuse::artifact)
 {
     item_tags = pitem_tags;
     artifact_itype_ids.push_back(pid);
 };
};

struct it_artifact_armor : public it_armor
{
 std::vector<art_effect_passive> effects_worn;

 virtual bool is_artifact()  { return true; }

 virtual picojson::value save_data()
 {
     std::map<std::string, picojson::value> data;

     data[std::string("type")] = picojson::value("artifact_armor");

     // generic data
     data[std::string("id")] = picojson::value(id);
     data[std::string("price")] = picojson::value(price);
     data[std::string("name")] = picojson::value(name);
     data[std::string("description")] = picojson::value(description);
     data[std::string("sym")] = picojson::value(sym);
     data[std::string("color")] = picojson::value(color_to_int(color));
     data[std::string("m1")] = picojson::value(m1);
     data[std::string("m2")] = picojson::value(m2);
     data[std::string("volume")] = picojson::value(volume);
     data[std::string("weight")] = picojson::value(weight);
     data[std::string("id")] = picojson::value(id);
     data[std::string("melee_dam")] = picojson::value(melee_dam);
     data[std::string("melee_cut")] = picojson::value(melee_cut);
     data[std::string("m_to_hit")] = picojson::value(m_to_hit);

     std::vector<picojson::value> tags_json;
     for(std::set<std::string>::iterator it = item_tags.begin();
         it != item_tags.end(); ++it)
     {
         tags_json.push_back(picojson::value(*it));
     }
     data[std::string("item_flags")] = picojson::value(tags_json);

     data[std::string("techniques")] = picojson::value(techniques);

     // armor data
     data[std::string("covers")] = picojson::value(covers);
     data[std::string("encumber")] = picojson::value(encumber);
     data[std::string("coverage")] = picojson::value(coverage);
     data[std::string("material_thickness")] = picojson::value(thickness);
     data[std::string("env_resist")] = picojson::value(env_resist);
     data[std::string("warmth")] = picojson::value(warmth);
     data[std::string("storage")] = picojson::value(storage);
     data[std::string("power_armor")] = picojson::value(power_armor);

     // artifact data
     std::vector<picojson::value> effects_worn_json;
     for(std::vector<art_effect_passive>::iterator it = effects_worn.begin();
         it != effects_worn.end(); ++it)
     {
         effects_worn_json.push_back(picojson::value(*it));
     }
     data[std::string("effects_worn")] =
         picojson::value(effects_worn_json);

     return picojson::value(data);
 }

 it_artifact_armor() :it_armor()
 {
  price = 0;
 };

 it_artifact_armor(std::string pid, unsigned int pprice, std::string pname,
                   std::string pdes, char psym, nc_color pcolor, std::string pm1,
                   std::string pm2, unsigned short pvolume, unsigned int pweight,
                   signed char pmelee_dam, signed char pmelee_cut,
                   signed char pm_to_hit, std::set<std::string> pitem_tags,

                   unsigned char pcovers, signed char pencumber,
                   unsigned char pcoverage, unsigned char pthickness,
                   unsigned char penv_resist, signed char pwarmth,
                   unsigned char pstorage)
:it_armor(pid, 0, pprice, pname, pdes, psym, pcolor, pm1, pm2,
          pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit,
          pcovers, pencumber, pcoverage, pthickness, penv_resist, pwarmth,
          pstorage)
 {
     item_tags = pitem_tags;
     artifact_itype_ids.push_back(pid);
 };
};

#endif
