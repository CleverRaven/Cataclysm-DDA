#ifndef _ITYPE_H_
#define _ITYPE_H_

#include <string>
#include <vector>
#include "color.h"
#include "enums.h"
#include "iuse.h"
#include "pldata.h"
#include "bodypart.h"
#include "skill.h"
#include "bionics.h"

// mfb(n) converts a flag to its appropriate position in covers's bitfield
#ifndef mfb
#define mfb(n) long(pow(2,(long)n))
#endif

enum itype_id {
itm_null = 0,
itm_corpse,
// Drinks
itm_water, itm_water_dirty, itm_salt_water, itm_oj, itm_energy_drink, itm_cola,
 itm_rootbeer, itm_milk, itm_V8, itm_whiskey, itm_vodka, itm_rum, itm_tequila,
 itm_beer, itm_bleach, itm_ammonia, itm_mutagen, itm_purifier,
// Monster Meats
itm_meat, itm_veggy, itm_meat_tainted, itm_veggy_tainted, itm_meat_cooked,
 itm_veggy_cooked,
// Food
itm_apple, itm_orange, itm_lemon, itm_chips, itm_pretzels, itm_chocolate,
 itm_jerky, itm_sandwich_t, itm_candy, itm_mushroom, itm_mushroom_poison,
 itm_mushroom_magic, itm_blueberries, itm_strawberries, itm_tomato,
 itm_broccoli, itm_zucchini, itm_frozen_dinner, itm_spaghetti_raw,
 itm_spaghetti_cooked, itm_macaroni_raw, itm_macaroni_cooked, itm_ravioli,
 itm_sauce_red, itm_sauce_pesto, itm_can_beans, itm_can_corn, itm_can_spam,
 itm_can_pineapple, itm_can_coconut, itm_can_sardine, itm_can_tuna,
 itm_can_catfood, itm_honeycomb, itm_royal_jelly, itm_fetus, itm_arm, itm_leg,
 itm_ant_egg, itm_marloss_berry,
// Medication
itm_bandages, itm_1st_aid, itm_vitamins, itm_aspirin, itm_caffeine,
 itm_pills_sleep, itm_iodine, itm_dayquil, itm_nyquil, itm_inhaler, itm_codeine,
 itm_oxycodone, itm_tramadol, itm_xanax, itm_adderall, itm_thorazine,
 itm_prozac, itm_cig, itm_weed, itm_coke, itm_meth, itm_heroin,
// Do-nothing / Melee weapons
itm_wrapper, itm_syringe, itm_rag, itm_fur, itm_leather, itm_superglue,
 itm_card_id, itm_electrohack, itm_string_6, itm_string_36, itm_rope_6,
 itm_rope_30, itm_chain, itm_processor, itm_RAM, itm_power_supply,
 itm_amplifier, itm_transponder, itm_receiver, itm_antenna, itm_steel_chunk,
 itm_motor, itm_hose, itm_glass_sheet, itm_manhole_cover, itm_rock, itm_stick,
 itm_broom, itm_mop, itm_screwdriver, itm_wrench, itm_saw, itm_hacksaw,
 itm_hammer_sledge, itm_hatchet, itm_ax, itm_nailboard, itm_xacto, itm_pot,
 itm_pan, itm_knife_butter, itm_knife_steak, itm_knife_butcher,
 itm_knife_combat, itm_2x4, itm_muffler, itm_pipe, itm_bat, itm_machete,
 itm_katana, itm_spear_wood, itm_spear_knife, itm_baton, itm_bee_sting,
 itm_wasp_sting, itm_chitin_piece, itm_canister_empty, itm_gold,
// Footwear
itm_sneakers, itm_boots, itm_boots_steel, itm_boots_winter, itm_mocassins,
 itm_flip_flops, itm_dress_shoes, itm_heels, 
// Legwear
itm_jeans, itm_pants, itm_pants_leather, itm_pants_cargo, itm_pants_army,
 itm_skirt,
// Full-body clothing
itm_jumpsuit, itm_dress, itm_armor_chitin,
// Torso clothing
itm_tshirt, itm_polo_shirt, itm_dress_shirt, itm_tank_top, itm_sweatshirt,
 itm_sweater, itm_hoodie, itm_jacket_light, itm_jacket_jean, itm_blazer,
 itm_jacket_leather, itm_kevlar, itm_coat_rain, itm_poncho, itm_trenchcoat,
 itm_coat_winter, itm_coat_fur, itm_peacoat, itm_vest, itm_coat_lab,
// Gloves
itm_gloves_light, itm_mittens, itm_gloves_wool, itm_gloves_winter,
 itm_gloves_leather, itm_gloves_fingerless, itm_gloves_rubber,
 itm_gloves_medical, itm_fire_gauntlets,
// Masks
itm_mask_dust, itm_bandana, itm_scarf, itm_mask_filter, itm_mask_gas,
// Eyewear
itm_glasses_eye, itm_glasses_reading, itm_glasses_safety, itm_goggles_swim,
 itm_goggles_ski, itm_goggles_welding, itm_goggles_nv,
// Headwear
itm_hat_ball, itm_hat_boonie, itm_hat_cotton, itm_hat_knit, itm_hat_hunting,
 itm_hat_fur, itm_hat_hard, itm_helmet_bike, itm_helmet_skid, itm_helmet_ball,
 itm_helmet_army, itm_helmet_riot, itm_helmet_motor, itm_helmet_chitin,
// High-storage
itm_backpack, itm_purse, itm_mbag, itm_fanny, itm_holster, itm_bootstrap,
// Ammunition
itm_battery, itm_plut_cell, itm_nail, itm_bb, itm_bolt_wood, itm_bolt_steel,
 itm_shot_bird, itm_shot_00, itm_shot_slug, itm_22_lr, itm_22_cb,
 itm_22_ratshot, itm_9mm, itm_9mmP, itm_9mmP2, itm_38_special, itm_38_super,
 itm_10mm, itm_40sw, itm_44magnum, itm_45_acp, itm_45_jhp, itm_45_super,
 itm_57mm, itm_46mm, itm_762_m43, itm_762_m87, itm_223, itm_556,
 itm_556_incendiary, itm_270, itm_3006, itm_3006_incendiary, itm_308,
 itm_762_51, itm_762_51_incendiary, itm_laser_pack, itm_gasoline,
// Guns
itm_nailgun, itm_bbgun, itm_crossbow, itm_rifle_22, itm_rifle_9mm, itm_smg_9mm,
 itm_smg_45, itm_sig_mosquito, itm_sw_22, itm_glock_19, itm_usp_9mm, itm_sw_619,
 itm_taurus_38, itm_sig_40, itm_sw_610, itm_ruger_redhawk, itm_deagle_44,
 itm_usp_45, itm_m1911, itm_fn57, itm_hk_ucp, itm_shotgun_sawn, itm_shotgun_s,
 itm_shotgun_d,  itm_remington_870, itm_mossberg_500, itm_saiga_12,
 itm_american_180, itm_uzi, itm_tec9, itm_calico, itm_hk_mp5, itm_mac_10,
 itm_hk_ump45, itm_TDI, itm_fn_p90, itm_hk_mp7, itm_marlin_9a, itm_ruger_1022,
 itm_browning_blr, itm_remington_700, itm_sks, itm_ruger_mini, itm_savage_111f,
 itm_hk_g3, itm_hk_g36, itm_ak47, itm_fn_fal, itm_acr, itm_ar15, itm_m4a1,
 itm_scar_l, itm_scar_h, itm_steyr_aug, itm_m249, itm_v29, itm_ftk93,
 itm_flamethrower_simple, itm_flamethrower,
// Gun modifications
itm_silencer, itm_grip, itm_barrel_big, itm_barrel_small, itm_barrel_rifled,
 itm_clip, itm_clip2, itm_stablizer, itm_blowback, itm_autofire, itm_retool_45,
 itm_retool_9mm, itm_retool_22, itm_retool_57, itm_retool_46, itm_retool_308,
 itm_retool_223, itm_conversion_battle, itm_conversion_sniper,
// Books
itm_mag_porn, itm_mag_tv, itm_mag_news, itm_mag_cars, itm_mag_cooking,
 itm_mag_guns, itm_novel_romance, itm_novel_spy, itm_novel_scifi,
 itm_novel_drama, itm_manual_brawl, itm_manual_knives, itm_manual_mechanics,
 itm_manual_speech, itm_manual_business, itm_manual_first_aid,
 itm_manual_computers, itm_cookbook, itm_manual_electronics,
 itm_manual_tailor, itm_manual_traps, itm_textbook_computers,
 itm_textbook_electronics, itm_textbook_business, itm_textbook_chemistry,
 itm_SICP, itm_textbook_robots,
// Containers
itm_bag_plastic, itm_bottle_plastic, itm_bottle_glass,
 itm_can_drink, itm_can_food, itm_box_small,
// Tools
itm_lighter, itm_sewing_kit, itm_scissors, itm_hammer, itm_extinguisher,
 itm_flashlight, itm_flashlight_on, itm_hotplate, itm_soldering_iron,
 itm_water_purifier, itm_two_way_radio, itm_radio, itm_radio_on, itm_crowbar,
 itm_hoe, itm_shovel, itm_chainsaw_off, itm_chainsaw_on, itm_jackhammer,
 itm_bubblewrap, itm_beartrap, itm_board_trap, itm_tripwire, itm_crossbow_trap,
 itm_shotgun_trap, itm_blade_trap, itm_landmine, itm_geiger_off, itm_geiger_on,
 itm_teleporter, itm_canister_goo, itm_pipebomb, itm_pipebomb_act, itm_grenade,
 itm_grenade_act, itm_EMPbomb, itm_EMPbomb_act, itm_gasbomb, itm_gasbomb_act,
 itm_smokebomb, itm_smokebomb_act, itm_molotov, itm_molotov_lit, itm_dynamite,
 itm_dynamite_act, itm_mininuke, itm_mininuke_act, itm_pheromone, itm_portal,
 itm_bot_manhack, itm_bot_turret, itm_UPS_off, itm_UPS_on, itm_tazer, itm_mp3,
 itm_mp3_on,
// Bionics containers
itm_bionics_battery,       itm_bionics_power,   itm_bionics_tools,
 itm_bionics_neuro,        itm_bionics_sensory, itm_bionics_aquatic,
 itm_bionics_combat_aug,   itm_bionics_hazmat,  itm_bionics_nutritional,
 itm_bionics_desert,       itm_bionics_melee,   itm_bionics_armor,
 itm_bionics_espionage,    itm_bionics_defense, itm_bionics_medical,
 itm_bionics_construction, itm_bionics_super,   itm_bionics_ranged,
// MacGuffins!
itm_note,
// Static (non-random) artifacts should go here.
num_items,
// These shouldn't be counted among "normal" items; thus, they are outside the
// bounds of num_items
itm_bio_claws, itm_bio_fusion, itm_bio_blaster,
num_all_items
};

enum item_cat {
IC_NULL,
IC_GUN, IC_AMMO, IC_WEAPON, IC_TOOL, IC_FOOD, IC_DRUG, IC_BOOK, IC_ARMOR,
IC_MISC
};

// IMPORTANT: If adding a new AT_*** ammotype, add it to the ammo_name function
//  at the end of itypedef.cpp
enum ammotype {
AT_NULL,
AT_BATT, AT_PLUT,
AT_NAIL, AT_BB, AT_BOLT,
AT_SHOT,
AT_22, AT_9MM, AT_38, AT_40, AT_44, AT_45,
AT_57, AT_46,
AT_762, AT_223, AT_3006, AT_308,
AT_GAS,
AT_FUSION,
NUM_AMMO_TYPES
};

enum weapon_flag {
WF_NULL,
WF_SPEAR,	// Cutting damage is actually a piercing attack
WF_STAB,	// This weapon *can* pierce, but also has normal cutting
WF_WRAP,	// Can wrap around your target, costing you and them movement
WF_MESSY,	// Splatters blood, etc.
WF_RELOAD_ONE,	// Reload cartridge by cartridge (e.g. most shotguns)

WF_AMMO_FLAME,	// Sets fire to terrain and monsters
WF_AMMO_INCENDIARY, // Sparks explosive terrain
WF_AMMO_EXPLOSIVE, // Small explosion
WF_AMMO_FRAG,	// Frag explosion
WF_AMMO_NAPALM,	// Firey explosion
WF_AMMO_EXPLOSIVE_BIG, // Big explosion!
WF_AMMO_SCENT,	// Scent burst
WF_AMMO_TRAIL,	// Leaves a trail of smoke
NUM_WEAPON_FLAGS
};

// Returns the name of a category of ammo (e.g. "shot")
std::string ammo_name(ammotype t);
// Returns the default ammo for a category of ammo (e.g. "itm_00_shot")
itype_id default_ammo(ammotype guntype);

struct itype
{
 unsigned int id;	// ID # that matches its place in master itype list
 			// Used for save files; aligns to itype_id above.
 unsigned char rarity;	// How often it's found
 unsigned int  price;	// Its value
 
 std::string name;	// Proper name
 std::string description;// Flavor text
 
 char sym;		// Symbol on the map
 nc_color color;	// Color on the map (color.h)
 
 material m1;		// Main material
 material m2;		// Secondary material -- MNULL if made of just 1 thing
 
 unsigned char volume;	// Space taken up by this item
 unsigned char weight;	// Weight in quarter-pounds; is 64 lbs max ok?
 			// Also assumes positive weight.  No helium, guys!
 
 signed char melee_dam;	// Bonus for melee damage; may be a penalty
 signed char melee_cut;	// Cutting damage in melee
 signed char m_to_hit;	// To-hit bonus for melee combat; -5 to 5 is reasonable

 unsigned weapon_flags : NUM_WEAPON_FLAGS;
 
 virtual bool is_food()      { return false; }
 virtual bool is_ammo()      { return false; }
 virtual bool is_gun()       { return false; }
 virtual bool is_gunmod()    { return false; }
 virtual bool is_bionic()    { return false; }
 virtual bool is_armor()     { return false; }
 virtual bool is_book()      { return false; }
 virtual bool is_tool()      { return false; }
 virtual bool is_container() { return false; }
 virtual bool is_macguffin() { return false; }

 itype() {
  id = 0;
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
  weapon_flags = 0;
 }
 
 itype(unsigned short pid, unsigned char prarity, unsigned int pprice,
       std::string pname, std::string pdes,
       char psym, nc_color pcolor, material pm1, material pm2,
       unsigned char pvolume, unsigned char pweight,
       signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
       unsigned pweapon_flags) {
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
  weapon_flags = pweapon_flags;
 }
};

// Includes food drink and drugs
struct it_comest : public itype
{
 signed char quench;	// Many things make you thirstier!
 unsigned char nutr;	// Nutrition imparted
 signed char spoils;	// How long it takes to spoil (hours / 600 turns)
 unsigned char addict;	// Addictiveness potential
 unsigned char charges;	// Defaults # of charges (drugs, loaf of bread? etc)
 signed char stim;
 signed char healthy;

 signed char fun;	// How fun its use is

 itype_id container;	// The container it comes in
 itype_id tool;		// Tool needed to consume (e.g. lighter for cigarettes)

 virtual bool is_food() { return true; }

 void (iuse::*use)(game *, player *, item *, bool);// Special effects of use
 add_type add;				// Effects of addiction

 it_comest(unsigned short pid, unsigned char prarity, unsigned int pprice,
           std::string pname, std::string pdes,
           char psym, nc_color pcolor, material pm1,
           unsigned char pvolume, unsigned char pweight,
           signed char pmelee_dam, signed char pmelee_cut,
           signed char pm_to_hit, unsigned pweapon_flags, 

           signed char pquench, unsigned char pnutr, signed char pspoils,
           signed char pstim, signed char phealthy, unsigned char paddict,
           unsigned char pcharges, signed char pfun, itype_id pcontainer,
           itype_id ptool, void (iuse::*puse)(game *, player *, item *, bool),
           add_type padd) 
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, MNULL,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pweapon_flags) {
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

 virtual bool is_ammo() { return true; }
 it_ammo(unsigned short pid, unsigned char prarity, unsigned int pprice,
        std::string pname, std::string pdes,
        char psym, nc_color pcolor, material pm1,
        unsigned char pvolume, unsigned char pweight,
        signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
        unsigned pweapon_flags,

        ammotype ptype, unsigned char pdamage, unsigned char ppierce,
	signed char paccuracy, unsigned char precoil, unsigned char prange,
        unsigned char pcount)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, MNULL,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pweapon_flags) {
  type = ptype;
  damage = pdamage;
  pierce = ppierce;
  range = prange;
  accuracy = paccuracy;
  recoil = precoil;
  count = pcount;
 }
};

struct it_gun : public itype
{
 ammotype ammo;
 skill skill_used;
 signed char dmg_bonus;
 signed char accuracy;
 signed char recoil;
 signed char durability;
 unsigned char burst;
 unsigned char clip;

 virtual bool is_gun() { return true; }

 it_gun(unsigned short pid, unsigned char prarity, unsigned int pprice,
        std::string pname, std::string pdes,
        char psym, nc_color pcolor, material pm1, material pm2,
        unsigned char pvolume, unsigned char pweight,
        signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
        unsigned pweapon_flags,

	skill pskill_used, ammotype pammo, signed char pdmg_bonus,
	signed char paccuracy, signed char precoil, unsigned char pdurability,
        unsigned char pburst, unsigned char pclip)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pweapon_flags) {
  skill_used = pskill_used;
  ammo = pammo;
  dmg_bonus = pdmg_bonus;
  accuracy = paccuracy;
  recoil = precoil;
  durability = pdurability;
  burst = pburst;
  clip = pclip;
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

 it_gunmod(unsigned short pid, unsigned char prarity, unsigned int pprice,
           std::string pname, std::string pdes,
           char psym, nc_color pcolor, material pm1, material pm2,
           unsigned char pvolume, unsigned char pweight,
           signed char pmelee_dam, signed char pmelee_cut,
           signed char pm_to_hit, unsigned pweapon_flags,

           signed char paccuracy, signed char pdamage, signed char ploudness,
           signed char pclip, signed char precoil, signed char pburst,
           ammotype pnewtype, long a_a_t, bool pistol,
           bool shotgun, bool smg, bool rifle)

 :itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
        pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pweapon_flags) {
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
 virtual bool is_armor() { return true; }
 it_armor(unsigned short pid, unsigned char prarity, unsigned int pprice,
          std::string pname, std::string pdes,
          char psym, nc_color pcolor, material pm1, material pm2,
          unsigned char pvolume, unsigned char pweight,
          signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
          unsigned pweapon_flags,

          unsigned char pcovers, signed char pencumber,
          unsigned char pdmg_resist, unsigned char pcut_resist,
          unsigned char penv_resist, signed char pwarmth,
          unsigned char pstorage)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pweapon_flags) {
  covers = pcovers;
  encumber = pencumber;
  dmg_resist = pdmg_resist;
  cut_resist = pcut_resist;
  env_resist = penv_resist;
  warmth = pwarmth;
  storage = pstorage;
 }
};

struct it_book : public itype
{
 skill type;		// Which skill it upgrades
 unsigned char level;	// The value it takes the skill to
 unsigned char req;	// The skill level required to understand it
 signed char fun;	// How fun reading this is
 unsigned char intel;	// Intelligence required to read, at all
 unsigned char time;	// How long, in 10-turns (aka minutes), it takes to read
			// "To read" means getting 1 skill point, not all of em
 virtual bool is_book() { return true; }
 it_book(unsigned short pid, unsigned char prarity, unsigned int pprice,
         std::string pname, std::string pdes,
         char psym, nc_color pcolor, material pm1, material pm2,
         unsigned char pvolume, unsigned char pweight,
         signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
         unsigned pweapon_flags,

	 skill ptype, unsigned char plevel, unsigned char preq,
	 signed char pfun, unsigned char pintel, unsigned char ptime)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pweapon_flags) {
  type = ptype;
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
 it_container(unsigned short pid, unsigned char prarity, unsigned int pprice,
              std::string pname, std::string pdes,
              char psym, nc_color pcolor, material pm1, material pm2,
              unsigned char pvolume, unsigned char pweight,
              signed char pmelee_dam, signed char pmelee_cut,
              signed char pm_to_hit, unsigned pweapon_flags,

              unsigned char pcontains, unsigned pflags)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pweapon_flags) {
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
 virtual bool is_tool() { return true; }
 it_tool(unsigned short pid, unsigned char prarity, unsigned int pprice,
         std::string pname, std::string pdes,
         char psym, nc_color pcolor, material pm1, material pm2,
         unsigned char pvolume, unsigned char pweight,
         signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
         unsigned pweapon_flags,

         unsigned int pmax_charges, unsigned int pdef_charges,
         unsigned char pcharges_per_use, unsigned char pturns_per_charge,
         ammotype pammo, itype_id prevert_to,
	 void (iuse::*puse)(game *, player *, item *, bool))
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pweapon_flags) {
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

 it_bionic(unsigned short pid, unsigned char prarity, unsigned int pprice,
           std::string pname, std::string pdes,
           char psym, nc_color pcolor, material pm1, material pm2,
           unsigned char pvolume, unsigned char pweight,
           signed char pmelee_dam, signed char pmelee_cut,
           signed char pm_to_hit, unsigned pweapon_flags,

           int pdifficulty, ...)
 :itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
        pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pweapon_flags) {
   difficulty = pdifficulty;
   va_list ap;
   va_start(ap, pdifficulty);
   bionic_id tmp;
   while (tmp = (bionic_id)va_arg(ap, int))
    options.push_back(tmp);
   va_end(ap);
 }
};

struct it_macguffin : public itype
{
 bool readable; // If true, activated with 'R'
 void (iuse::*use)(game *, player *, item *, bool);
 
 virtual bool is_macguffin() { return true; }

 it_macguffin(unsigned short pid, unsigned char prarity, unsigned int pprice,
              std::string pname, std::string pdes,
              char psym, nc_color pcolor, material pm1, material pm2,
              unsigned char pvolume, unsigned char pweight,
              signed char pmelee_dam, signed char pmelee_cut,
              signed char pm_to_hit, unsigned pweapon_flags,

              bool preadable,
              void (iuse::*puse)(game *, player *, item *, bool))
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pweapon_flags) {
  readable = preadable;
  use = puse;
 }
};

#endif
