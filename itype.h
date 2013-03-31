#ifndef _ITYPE_H_
#define _ITYPE_H_

#include <string>
#include <vector>
#include <sstream>
#include "color.h"
#include "enums.h"
#include "iuse.h"
#include "pldata.h"
#include "bodypart.h"
#include "skill.h"
#include "bionics.h"
#include "artifact.h"

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

// IMPORTANT: If adding a new AT_*** ammotype, add it to the ammo_name function
//  at the end of itypedef.cpp
enum ammotype {
AT_NULL, AT_THREAD,
AT_BATT, AT_PLUT,
AT_NAIL, AT_BB, AT_BOLT, AT_ARROW,
AT_SHOT,
AT_22, AT_9MM, AT_762x25, AT_38, AT_40, AT_44, AT_45,
AT_57, AT_46,
AT_762, AT_223, AT_3006, AT_308,
AT_40MM,
AT_GAS,
AT_FUSION,
AT_MUSCLE,
AT_12MM,
AT_PLASMA,
AT_WATER,
NUM_AMMO_TYPES
};

enum software_type {
SW_NULL,
SW_USELESS,
SW_HACKING,
SW_MEDICAL,
SW_SCIENCE,
SW_DATA,
NUM_SOFTWARE_TYPES
};

enum item_flag {
IF_NULL,

IF_LIGHT_4,	// Provides 4 tiles of light
IF_LIGHT_8,	// Provides 8 tiles of light

IF_FIRE,        // Chance to set fire to tiles/enemies
IF_SPEAR,	// Cutting damage is actually a piercing attack
IF_STAB,	// This weapon *can* pierce, but also has normal cutting
IF_WRAP,	// Can wrap around your target, costing you and them movement
IF_MESSY,	// Splatters blood, etc.
IF_RELOAD_ONE,	// Reload cartridge by cartridge (e.g. most shotguns)
IF_STR_RELOAD,  // Reloading time is reduced by Strength * 20
IF_STR8_DRAW,   // Requires strength 8 to draw
IF_STR10_DRAW,  // Requires strength 10 to draw
IF_USE_UPS,	// Draws power from a UPS
IF_RELOAD_AND_SHOOT, // Reloading and shooting is one action
IF_FIRE_100,	// Fires 100 rounds at once! (e.g. flamethrower)
IF_GRENADE,	// NPCs treat this as a grenade
IF_CHARGE,	// For guns; charges up slowly
IF_SHOCK,   // Stuns and damages enemies, powers up shockers.

IF_UNARMED_WEAPON, // Counts as an unarmed weapon
IF_NO_UNWIELD, // Impossible to unwield, e.g. bionic claws

// Weapon mode flags
IF_MODE_AUX, // A gunmod with a firing mode
IF_MODE_BURST, // A burst of attacks

// Food status flags
IF_HOT,				// hot food
IF_EATEN_HOT,	// food meant to be eaten hot
IF_ROTTEN, 		// rotten foox

NUM_ITEM_FLAGS
};

enum ammo_effect {
AMMO_FLAME,		// Sets fire to terrain and monsters
AMMO_INCENDIARY,	// Sparks explosive terrain
AMMO_EXPLOSIVE,		// Small explosion
AMMO_FRAG,		// Frag explosion
AMMO_NAPALM,		// Firey explosion
AMMO_EXPLOSIVE_BIG,	// Big explosion!
AMMO_TEARGAS,		// Teargas burst
AMMO_SMOKE,  		// Smoke burst
AMMO_TRAIL,		// Leaves a trail of smoke
AMMO_FLASHBANG,		// Disorients and blinds
AMMO_STREAM,		// Doesn't stop once it hits a monster
NUM_AMMO_EFFECTS
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
 technique_id tech;
 int level;

 style_move(std::string N, technique_id T, int L) :
  name (N), tech (T), level (L) { };

 style_move()
 {
  name = "";
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

 material m1;		// Main material
 material m2;		// Secondary material -- MNULL if made of just 1 thing

 unsigned int volume;	// Space taken up by this item
 unsigned int weight;	// Weight in quarter-pounds; is 64 lbs max ok?
 			// Also assumes positive weight.  No helium, guys!
 bigness_property_aspect bigness_aspect;

 signed char melee_dam;	// Bonus for melee damage; may be a penalty
 signed char melee_cut;	// Cutting damage in melee
 signed char m_to_hit;	// To-hit bonus for melee combat; -5 to 5 is reasonable

 unsigned item_flags : NUM_ITEM_FLAGS;
 unsigned techniques : NUM_TECHNIQUES;

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
 virtual bool is_artifact()      { return false; }
 virtual bool is_var_veh_part()  { return false; }
 virtual bool is_engine()         { return false; }
 virtual bool is_wheel()          { return false; }
 virtual bool count_by_charges() { return false; }
 virtual std::string save_data() { return std::string(); }

 itype() {
  id = "null";
  rarity = 0;
  name  = "none";
  sym = '#';
  color = c_white;
  m1 = MNULL;
  m2 = MNULL;
  volume = 0;
  weight = 0;
  melee_dam = 0;
  m_to_hit = 0;
  item_flags = 0;
  techniques = 0;
 }

 itype(std::string pid, unsigned char prarity, unsigned int pprice,
       std::string pname, std::string pdes,
       char psym, nc_color pcolor, material pm1, material pm2,
       unsigned short pvolume, unsigned short pweight,
       signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
       unsigned pitem_flags, unsigned ptechniques = 0) {
  id          = pid;
  rarity      = prarity;
  price       = pprice;
  name        = pname;
  description = pdes;
  sym         = psym;
  color       = pcolor;
  m1          = pm1;
  m2          = pm2;
  volume      = pvolume;
  weight      = pweight;
  melee_dam   = pmelee_dam;
  melee_cut   = pmelee_cut;
  m_to_hit    = pm_to_hit;
  item_flags  = pitem_flags;
  techniques  = ptechniques;
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
        if (m1 == LIQUID) {
            return true;
        } else {
            return charges > 1 ;
        }
    }

    void (iuse::*use)(game *, player *, item *, bool);// Special effects of use
    add_type add;				// Effects of addiction

    it_comest(std::string pid, unsigned char prarity, unsigned int pprice,
    std::string pname, std::string pdes,
    char psym, nc_color pcolor, material pm1,
    unsigned short pvolume, unsigned short pweight,
    signed char pmelee_dam, signed char pmelee_cut,
    signed char pm_to_hit, unsigned pitem_flags,

    signed char pquench, unsigned char pnutr, signed char pspoils,
    signed char pstim, signed char phealthy, unsigned char paddict,
    unsigned char pcharges, signed char pfun, itype_id pcontainer,
    itype_id ptool, void (iuse::*puse)(game *, player *, item *, bool),
    add_type padd, std::string pcomesttype)
    :itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, MNULL,
    pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) {
        quench = pquench;
        nutr = pnutr;
        spoils = pspoils;
        stim = pstim;
        healthy = phealthy;
        addict = paddict;
        charges = pcharges;
        fun = pfun;
        container = pcontainer;
        tool = ptool;
        use = puse;
        add = padd;
        comesttype = pcomesttype;
        item_flags = pitem_flags;
    }
};

// v6, v8, wankel, etc.
struct it_var_veh_part: public itype
{
 // TODO? geometric mean: nth root of product
 unsigned int min_bigness; //CC's
 unsigned int max_bigness;

 it_var_veh_part(std::string pid, unsigned char prarity, unsigned int pprice,
        std::string pname, std::string pdes,
        char psym, nc_color pcolor, material pm1, material pm2,
        unsigned short pvolume, unsigned short pweight,
        signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
        unsigned effects,

        unsigned int big_min,
        unsigned int big_max,
        bigness_property_aspect big_aspect)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, 0) {
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
 signed char accuracy;	// Accuracy (low is good)
 unsigned char recoil;	// Recoil; modified by strength
 unsigned char count;	// Default charges

 unsigned ammo_effects : NUM_AMMO_EFFECTS;

 virtual bool is_ammo() { return true; }
// virtual bool count_by_charges() { return id != "gasoline"; }
 virtual bool count_by_charges() { return true; }

 it_ammo(std::string pid, unsigned char prarity, unsigned int pprice,
        std::string pname, std::string pdes,
        char psym, nc_color pcolor, material pm1,
        unsigned short pvolume, unsigned short pweight,
        signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
        unsigned effects,

        ammotype ptype, unsigned char pdamage, unsigned char ppierce,
	signed char paccuracy, unsigned char precoil, unsigned char prange,
        unsigned char pcount)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, MNULL,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, 0) {
  type = ptype;
  damage = pdamage;
  pierce = ppierce;
  range = prange;
  accuracy = paccuracy;
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
 signed char accuracy;
 signed char recoil;
 signed char durability;
 unsigned char burst;
 int clip;
 int reload_time;

 virtual bool is_gun() { return true; }

 it_gun(std::string pid, unsigned char prarity, unsigned int pprice,
        std::string pname, std::string pdes,
        char psym, nc_color pcolor, material pm1, material pm2,
        unsigned short pvolume, unsigned short pweight,
        signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
        unsigned pitem_flags,

	const char *pskill_used, ammotype pammo, signed char pdmg_bonus,
	signed char paccuracy, signed char precoil, unsigned char pdurability,
        unsigned char pburst, int pclip, int preload_time)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) {
  skill_used = pskill_used?Skill::skill(pskill_used):NULL;
  ammo = pammo;
  dmg_bonus = pdmg_bonus;
  accuracy = paccuracy;
  recoil = precoil;
  durability = pdurability;
  burst = pburst;
  clip = pclip;
  reload_time = preload_time;
 }
};

struct it_gunmod : public itype
{
 signed char accuracy, damage, loudness, clip, recoil, burst;
 ammotype newtype;
 unsigned acceptible_ammo_types : NUM_AMMO_TYPES;
 bool used_on_pistol;
 bool used_on_shotgun;
 bool used_on_smg;
 bool used_on_rifle;

 virtual bool is_gunmod() { return true; }

 it_gunmod(std::string pid, unsigned char prarity, unsigned int pprice,
           std::string pname, std::string pdes,
           char psym, nc_color pcolor, material pm1, material pm2,
           unsigned short pvolume, unsigned short pweight,
           signed char pmelee_dam, signed char pmelee_cut,
           signed char pm_to_hit, unsigned pitem_flags,

           signed char paccuracy, signed char pdamage, signed char ploudness,
           signed char pclip, signed char precoil, signed char pburst,
           ammotype pnewtype, long a_a_t, bool pistol,
           bool shotgun, bool smg, bool rifle)

 :itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
        pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) {
  accuracy = paccuracy;
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
};

struct it_armor : public itype
{
 unsigned char covers; // Bitfield of enum body_part
 signed char encumber;
 unsigned char dmg_resist;
 unsigned char cut_resist;
 unsigned char env_resist; // Resistance to environmental effects
 signed char warmth;
 unsigned char storage;

 bool power_armor;

 virtual bool is_armor() { return true; }
 virtual bool is_power_armor() { return power_armor; }
 virtual bool is_artifact() { return false; }
 virtual std::string save_data() { return std::string(); }

 it_armor()
 {
  covers = 0;
  encumber = 0;
  dmg_resist = 0;
  cut_resist = 0;
  env_resist = 0;
  warmth = 0;
  storage = 0;
 }

 it_armor(itype_id pid, unsigned char prarity, unsigned int pprice,
          std::string pname, std::string pdes,
          char psym, nc_color pcolor, material pm1, material pm2,
          unsigned short pvolume, unsigned short pweight,
          signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
          unsigned pitem_flags,

          unsigned char pcovers, signed char pencumber,
          unsigned char pdmg_resist, unsigned char pcut_resist,
          unsigned char penv_resist, signed char pwarmth,
          unsigned char pstorage, bool ppower_armor = false)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) {
  covers = pcovers;
  encumber = pencumber;
  dmg_resist = pdmg_resist;
  cut_resist = pcut_resist;
  env_resist = penv_resist;
  warmth = pwarmth;
  storage = pstorage;
  power_armor = ppower_armor;
 }
};

struct it_book : public itype
{
 Skill *type;		// Which skill it upgrades
 unsigned char level;	// The value it takes the skill to
 unsigned char req;	// The skill level required to understand it
 signed char fun;	// How fun reading this is
 unsigned char intel;	// Intelligence required to read, at all
 unsigned char time;	// How long, in 10-turns (aka minutes), it takes to read
			// "To read" means getting 1 skill point, not all of em
 virtual bool is_book() { return true; }
 it_book(std::string pid, unsigned char prarity, unsigned int pprice,
         std::string pname, std::string pdes,
         char psym, nc_color pcolor, material pm1, material pm2,
         unsigned short pvolume, unsigned short pweight,
         signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
         unsigned pitem_flags,

	 const char *ptype, unsigned char plevel, unsigned char preq,
	 signed char pfun, unsigned char pintel, unsigned char ptime)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) {
  type = ptype?Skill::skill(ptype):NULL;
  level = plevel;
  req = preq;
  fun = pfun;
  intel = pintel;
  time = ptime;
 }
};

enum container_flags {
 con_rigid,
 con_wtight,
 con_seals,
 num_con_flags
};

struct it_container : public itype
{
 unsigned char contains;	// Internal volume
 unsigned flags : num_con_flags;
 virtual bool is_container() { return true; }
 it_container(std::string pid, unsigned char prarity, unsigned int pprice,
              std::string pname, std::string pdes,
              char psym, nc_color pcolor, material pm1, material pm2,
              unsigned short pvolume, unsigned short pweight,
              signed char pmelee_dam, signed char pmelee_cut,
              signed char pm_to_hit, unsigned pitem_flags,

              unsigned char pcontains, unsigned pflags)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) {
  contains = pcontains;
  flags = pflags;
 }
};

struct it_tool : public itype
{
 ammotype ammo;
 unsigned int max_charges;
 unsigned int def_charges;
 unsigned char charges_per_use;
 unsigned char turns_per_charge;
 itype_id revert_to;
 void (iuse::*use)(game *, player *, item *, bool);

 virtual bool is_tool()          { return true; }
 virtual bool is_artifact()      { return false; }
 virtual std::string save_data() { return std::string(); }

 it_tool()
 {
  ammo = AT_NULL;
  max_charges = 0;
  def_charges = 0;
  charges_per_use = 0;
  turns_per_charge = 0;
  revert_to = "null";
  use = &iuse::none;
 }

 it_tool(std::string pid, unsigned char prarity, unsigned int pprice,
         std::string pname, std::string pdes,
         char psym, nc_color pcolor, material pm1, material pm2,
         unsigned short pvolume, unsigned short pweight,
         signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
         unsigned pitem_flags,

         unsigned int pmax_charges, unsigned int pdef_charges,
         unsigned char pcharges_per_use, unsigned char pturns_per_charge,
         ammotype pammo, itype_id prevert_to,
	 void (iuse::*puse)(game *, player *, item *, bool))
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) {
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
           char psym, nc_color pcolor, material pm1, material pm2,
           unsigned short pvolume, unsigned short pweight,
           signed char pmelee_dam, signed char pmelee_cut,
           signed char pm_to_hit, unsigned pitem_flags,

           int pdifficulty)
 :itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
        pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) {
   difficulty = pdifficulty;
   options.push_back(id);
 }
};

struct it_macguffin : public itype
{
 bool readable; // If true, activated with 'R'
 void (iuse::*use)(game *, player *, item *, bool);

 virtual bool is_macguffin() { return true; }

 it_macguffin(std::string pid, unsigned char prarity, unsigned int pprice,
              std::string pname, std::string pdes,
              char psym, nc_color pcolor, material pm1, material pm2,
              unsigned short pvolume, unsigned short pweight,
              signed char pmelee_dam, signed char pmelee_cut,
              signed char pm_to_hit, unsigned pitem_flags,

              bool preadable,
              void (iuse::*puse)(game *, player *, item *, bool))
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) {
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
             char psym, nc_color pcolor, material pm1, material pm2,
             unsigned short pvolume, unsigned short pweight,
             signed char pmelee_dam, signed char pmelee_cut,
             signed char pm_to_hit, unsigned pitem_flags,

             software_type pswtype, int ppower)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) {
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
          char psym, nc_color pcolor, material pm1, material pm2,
          unsigned char pvolume, unsigned char pweight,
          signed char pmelee_dam, signed char pmelee_cut,
          signed char pm_to_hit, unsigned pitem_flags)

:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) { }
};

struct it_artifact_tool : public it_tool
{
 art_charge charge_type;
 std::vector<art_effect_passive> effects_wielded;
 std::vector<art_effect_active>  effects_activated;
 std::vector<art_effect_passive> effects_carried;

 virtual bool is_artifact()  { return true; }
 virtual std::string save_data()
 {
  std::stringstream data;
  data << "T " << price << " " << sym << " " << color_to_int(color) << " " <<
          int(m1) << " " << int(m2) << " " << int(volume) << " " <<
          int(weight) << " " << int(melee_dam) << " " << int(melee_cut) <<
          " " << int(m_to_hit) << " " << int(item_flags) << " " <<
          int(charge_type) << " " << max_charges << " " <<
          effects_wielded.size();
  for (int i = 0; i < effects_wielded.size(); i++)
   data << " " << int(effects_wielded[i]);

  data << " " << effects_activated.size();
  for (int i = 0; i < effects_activated.size(); i++)
   data << " " << int(effects_activated[i]);

  data << " " << effects_carried.size();
  for (int i = 0; i < effects_carried.size(); i++)
   data << " " << int(effects_carried[i]);

  data << " " << name << " - ";
  std::string desctmp = description;
  size_t endline;
  do {
   endline = desctmp.find("\n");
   if (endline != std::string::npos)
    desctmp.replace(endline, 1, " = ");
  } while (endline != std::string::npos);
  data << desctmp << " -";
  return data.str();
 }

 it_artifact_tool() {
  ammo = AT_NULL;
  price = 0;
  def_charges = 0;
  charges_per_use = 1;
  turns_per_charge = 0;
  revert_to = "null";
  use = &iuse::artifact;
 };

 it_artifact_tool(std::string pid, unsigned int pprice, std::string pname,
                  std::string pdes, char psym, nc_color pcolor, material pm1,
                  material pm2, unsigned short pvolume, unsigned short pweight,
                  signed char pmelee_dam, signed char pmelee_cut,
                  signed char pm_to_hit, unsigned pitem_flags)

:it_tool(pid, 0, pprice, pname, pdes, psym, pcolor, pm1, pm2,
         pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags,
         0, 0, 1, 0, AT_NULL, "null", &iuse::artifact) { };
};

struct it_artifact_armor : public it_armor
{
 std::vector<art_effect_passive> effects_worn;

 virtual bool is_artifact()  { return true; }

 virtual std::string save_data()
 {
  std::stringstream data;
  data << "A " << price << " " << sym << " " << color_to_int(color) << " " <<
          int(m1) << " " << int(m2) << " " << int(volume) << " " <<
          int(weight) << " " << int(melee_dam) << " " << int(melee_cut) <<
          " " << int(m_to_hit) << " " << int(item_flags) << " " <<
          int(covers) << " " << int(encumber) << " " << int(dmg_resist) <<
          " " << int(cut_resist) << " " << int(env_resist) << " " <<
          int(warmth) << " " << int(storage) << " " << effects_worn.size();
  for (int i = 0; i < effects_worn.size(); i++)
   data << " " << int(effects_worn[i]);

  data << " " << name << " - ";
  std::string desctmp = description;
  size_t endline;
  do {
   endline = desctmp.find("\n");
   if (endline != std::string::npos)
    desctmp.replace(endline, 1, " = ");
  } while (endline != std::string::npos);

  data << desctmp << " -";

  return data.str();
 }

 it_artifact_armor()
 {
  price = 0;
 };

 it_artifact_armor(std::string pid, unsigned int pprice, std::string pname,
                   std::string pdes, char psym, nc_color pcolor, material pm1,
                   material pm2, unsigned short pvolume, unsigned short pweight,
                   signed char pmelee_dam, signed char pmelee_cut,
                   signed char pm_to_hit, unsigned pitem_flags,

                   unsigned char pcovers, signed char pencumber,
                   unsigned char pdmg_resist, unsigned char pcut_resist,
                   unsigned char penv_resist, signed char pwarmth,
                   unsigned char pstorage)
:it_armor(pid, 0, pprice, pname, pdes, psym, pcolor, pm1, pm2,
          pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags,
          pcovers, pencumber, pdmg_resist, pcut_resist, penv_resist, pwarmth,
          pstorage) { };
};

#endif
