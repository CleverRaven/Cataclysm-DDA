#include "itype.h"
#include "game.h"
#include "setvector.h"
#include "monstergenerator.h"
#include <fstream>

// Armor colors
#define C_SHOES  c_blue
#define C_PANTS  c_brown
#define C_BODY   c_yellow
#define C_TORSO  c_ltred
#define C_ARMS   c_blue
#define C_GLOVES c_ltblue
#define C_MOUTH  c_white
#define C_EYES   c_cyan
#define C_HAT    c_dkgray
#define C_STORE  c_green
#define C_DECOR  c_ltgreen

// Special function for setting melee techniques
#define TECH(id, t) itypes[id]->techniques = t

std::vector<std::string> unreal_itype_ids;
std::vector<std::string> martial_arts_itype_ids;
std::vector<std::string> artifact_itype_ids;
std::vector<std::string> standard_itype_ids;
std::vector<std::string> pseudo_itype_ids;

std::map<std::string, itype*> itypes;

// GENERAL GUIDELINES
// When adding a new item, you MUST REMEMBER to insert it in the itype_id enum
//  at the top of itype.h!
//  Additionally, you should check mapitemsdef.cpp and insert the new item in
//  any appropriate lists.
void game::init_itypes ()
{
// First, the null object.  NOT REALLY AN OBJECT AT ALL.  More of a concept.
 itypes["null"]=
  new itype("null", 0, "none", "", '#', c_white, "null", "null", PNULL, 0, 0, 0, 0, 0);
// Corpse - a special item
 itypes["corpse"]=
  new itype("corpse", 0, _("corpse"), _("A dead body."), '%', c_white, "null", "null", PNULL, 0, 0,
            0, 0, 1);
 itypes["corpse"]->item_tags.insert("NO_UNLOAD");
// This must -always- be set, or bad mojo in map::drawsq and whereever we check 'typeId() == "corpse" instead of 'corpse != NULL' ....
 itypes["corpse"]->corpse=GetMType("mon_null");
// Fire - only appears in crafting recipes
 itypes["fire"]=
  new itype("fire", 0, _("nearby fire"),
            "Some fire - if you are reading this it's a bug! (itypdef:fire)",
            '$', c_red, "null", "null", PNULL, 0, 0, 0, 0, 0);
// Integrated toolset - ditto
 itypes["toolset"]=
  new itype("toolset", 0, _("integrated toolset"),
            "A fake item. If you are reading this it's a bug! (itypdef:toolset)",
            '$', c_red, "null", "null", PNULL, 0, 0, 0, 0, 0);
// For smoking drugs
 itypes["apparatus"]=
  new itype("apparatus", 0, _("a smoking device and a source of flame"),
            "A fake item. If you are reading this it's a bug! (itypdef:apparatus)",
            '$', c_red, "null", "null", PNULL, 0, 0, 0, 0, 0);

#define VAR_VEH_PART(id, name, price, sym, color, mat1, mat2, volume, wgt,\
                     dam, cut, to_hit, bigmin, bigmax, bigaspect, des)\
itypes[id]=new it_var_veh_part(id, price, name, des, sym, color, mat1, mat2,\
                     volume, wgt, dam, cut, to_hit, bigmin, bigmax, bigaspect)

//"wheel", "wheel_wide", "wheel_bicycle", "wheel_motorbike", "wheel_small",
//           NAME     RAR PRC  SYM COLOR        MAT1    MAT2
VAR_VEH_PART("wheel", _("wheel"), 100, ']', c_dkgray,  "steel",   "plastic",
//  VOL WGT  DAM  CUT  HIT  BIGNESS_MIN BIGNESS_MAX  BIGNESS_ASPECT
    40, 8845, 12,  0,  -1,        13,         20,  BIGNESS_WHEEL_DIAMETER,
  _("A car wheel"));
//           NAME         RAR PRC  SYM COLOR        MAT1    MAT2
VAR_VEH_PART("wheel_wide", _("wide wheel"), 340, ']', c_dkgray,  "steel",   "plastic",
//  VOL WGT  DAM  CUT  HIT  BIGNESS_MIN BIGNESS_MAX  BIGNESS_ASPECT
    70,22600, 17,  0,  -1,        17,         36,  BIGNESS_WHEEL_DIAMETER,
  _("A wide wheel. \\o/ This wide."));
//           NAME            RAR  PRC  SYM COLOR        MAT1    MAT2
VAR_VEH_PART("wheel_bicycle", _("bicycle wheel"), 40,  ']', c_dkgray,  "steel",   "plastic",
//  VOL WGT  DAM  CUT  HIT  BIGNESS_MIN BIGNESS_MAX  BIGNESS_ASPECT
    28,1500,  8,   0,  -1,        9,         18,  BIGNESS_WHEEL_DIAMETER,
  _("A bicycle wheel"));
//           NAME              RAR  PRC   SYM COLOR        MAT1    MAT2
VAR_VEH_PART("wheel_motorbike", _("motorbike wheel"), 140,  ']', c_dkgray,  "steel",   "plastic",
//  VOL WGT  DAM  CUT  HIT  BIGNESS_MIN BIGNESS_MAX  BIGNESS_ASPECT
    33,5443,  10,  0,  -1,        9,         14,  BIGNESS_WHEEL_DIAMETER,
  _("A motorbike wheel"));
//           NAME              RAR  PRC   SYM COLOR        MAT1    MAT2
VAR_VEH_PART("wheel_small", _("small wheel"),140,  ']', c_dkgray,  "steel",   "plastic",
//  VOL WGT  DAM  CUT  HIT  BIGNESS_MIN BIGNESS_MAX  BIGNESS_ASPECT
    9, 2722,  10,  0,  -1,        6,         14,   BIGNESS_WHEEL_DIAMETER,
  _("A pretty small wheel. Probably from one of those segway things.\
It is not very menacing."));
VAR_VEH_PART("wheel_caster", _("casters"),140,  ']', c_dkgray,  "steel",   "plastic",
//  VOL WGT  DAM  CUT  HIT  BIGNESS_MIN BIGNESS_MAX  BIGNESS_ASPECT
    5, 1500,  6,   0,  -1,        4,         6,   BIGNESS_WHEEL_DIAMETER,
  _("A set of casters, like on a shopping cart."));

//                                 NAME           RAR PRC SYM COLOR        MAT1    MAT2
VAR_VEH_PART("1cyl_combustion", _("1-cylinder engine"), 100, ':', c_ltcyan,  "iron",   "null",
//  VOL WGT  DAM  CUT  HIT  BIGNESS_MIN BIGNESS_MAX  BIGNESS_ASPECT
    6, 20000,  4,  0,  -1,        28,         75,   BIGNESS_ENGINE_DISPLACEMENT, _("\
A single-cylinder 4-stroke combustion engine."));

//                              NAME           RAR PRC SYM COLOR        MAT1    MAT2
VAR_VEH_PART("v2_combustion", _("V-twin engine"), 100, ':', c_ltcyan,  "iron",   "null",
//  VOL WGT  DAM  CUT  HIT  BIGNESS_MIN BIGNESS_MAX  BIGNESS_ASPECT
    6, 45000,  4,  0,  -1,        65,        260, BIGNESS_ENGINE_DISPLACEMENT, _("\
A 2-cylinder 4-stroke combustion engine."));

//                                NAME           RAR PRC SYM COLOR        MAT1    MAT2
VAR_VEH_PART("i4_combustion", _("Inline-4 engine"), 150, ':', c_ltcyan,  "iron",   "null",
//  VOL WGT  DAM  CUT  HIT  BIGNESS_MIN BIGNESS_MAX  BIGNESS_ASPECT
    6, 70000,  8,  0,  -2,        220,       350, BIGNESS_ENGINE_DISPLACEMENT, _("\
A small, yet powerful 4-cylinder combustion engine."));

//                          NAME           RAR PRC SYM COLOR        MAT1    MAT2
VAR_VEH_PART("v6_combustion", _("V6 engine"), 180, ':', c_ltcyan,  "iron",   "null",
//  VOL WGT  DAM  CUT  HIT  BIGNESS_MIN BIGNESS_MAX  BIGNESS_ASPECT
    14,100000, 12,  0, -3,      250,        520, BIGNESS_ENGINE_DISPLACEMENT,
    _("A powerful 6-cylinder combustion engine."));

//                          NAME           RAR PRC SYM COLOR        MAT1    MAT2
VAR_VEH_PART("v8_combustion", _("V8 engine"), 250, ':', c_ltcyan,  "iron",   "null",
//  VOL WGT  DAM  CUT  HIT  BIGNESS_MIN BIGNESS_MAX  BIGNESS_ASPECT
    25,144000, 15,  0, -5,       380,     700, BIGNESS_ENGINE_DISPLACEMENT,
    _("A large and very powerful 8-cylinder combustion engine."));

// GUNS
// ammo_type matches one of the ammo_types above.
// dmg is ADDED to the damage of the corresponding ammo.  +/-, should be small.
// aim affects chances of hitting; low for handguns, hi for rifles, etc, small.
// Durability is rated 1-10; 10 near perfect, 1 it breaks every few shots
// Burst is the # of rounds fired, 0 if no burst ability.
// clip is how many shots we get before reloading.

#define GUN(id,name,price,color,mat1,mat2,skill,ammo,volume,wgt,melee_dam,\
to_hit,dmg,range,dispersion,recoil,durability,burst,clip,reload_time,des,pierce,flags,effects) \
itypes[id]=new it_gun(id,price,name,des,'(',\
color,mat1,mat2,volume,wgt,melee_dam,0,to_hit,pierce,flags,effects,\
skill,ammo,dmg,range,dispersion,\
recoil,durability,burst,clip,reload_time)

// BIONICS
// These are the modules used to install new bionics in the player.  They're
// very simple and straightforward; a difficulty, followed by a NULL-terminated
// list of options.
#define BIO(id, name, price, color, difficulty, des) \
itypes[id]=new it_bionic(id, price,name,des,':',\
color, "steel", "plastic", 10,2041, 8, 0, 0, difficulty)

#define BIO_SINGLE(id,price,color,difficulty) \
     BIO(id, std::string(_("CBM: "))+bionics[id]->name, price,color,difficulty, \
           bionics[id]->description) \

// SOFTWARE
#define SOFTWARE(id, name, price, swtype, power, description) \
itypes[id]=new it_software(id, price, name, description,\
    ' ', c_white, "null", "null", 0, 0, 0, 0, 0, swtype, power)

//Macguffins
#define MACGUFFIN(id, name, price, sym, color, mat1, mat2, volume, wgt, dam, cut,\
                  to_hit, readable, function, description) \
itypes[id]=new it_macguffin(id, price, name, description,\
    sym, color, mat1, mat2, volume, wgt, dam, cut, to_hit, readable,\
    function)

// BIONIC IMPLANTS
// Sometimes a bionic needs to set you up with a dummy weapon, or something
// similar.  For the sake of clarity, no matter what the type of item, place
// them all here.

// TODO: move the cost, color and difficulty to bionics.json

// power storage
BIO_SINGLE("bio_power_storage", 3800, c_green, 1);
BIO_SINGLE("bio_power_storage_mkII", 10000, c_green, 1);
// power sources
BIO_SINGLE("bio_solar", 3500, c_yellow, 4);
BIO_SINGLE("bio_batteries", 800, c_yellow, 4);
BIO_SINGLE("bio_metabolics", 700, c_yellow, 4);
BIO_SINGLE("bio_furnace", 4500, c_yellow, 4);
BIO_SINGLE("bio_ethanol", 1200, c_yellow, 4);
BIO_SINGLE("bio_torsionratchet", 3800, c_yellow, 4);
// utilities
BIO_SINGLE("bio_tools", 8000, c_ltgray, 6);
BIO_SINGLE("bio_storage", 4000, c_ltgray, 7);
BIO_SINGLE("bio_flashlight", 200, c_ltgray, 2);
BIO_SINGLE("bio_lighter", 1300, c_ltgray, 4);
BIO_SINGLE("bio_magnet", 2000, c_ltgray, 2);
// neurological
BIO_SINGLE("bio_memory", 10000, c_pink, 9);
BIO_SINGLE("bio_painkiller", 2000, c_pink, 4);
BIO_SINGLE("bio_alarm", 250, c_pink, 1);
BIO_SINGLE("bio_int_enhancer", 8000, c_ltblue, 11);
// sensory
BIO_SINGLE("bio_ears", 5000, c_ltblue, 6);
BIO_SINGLE("bio_eye_enhancer", 8000, c_ltblue, 11);
BIO_SINGLE("bio_night_vision", 9000, c_ltblue, 11);
BIO_SINGLE("bio_infrared", 4500, c_ltblue, 6);
BIO_SINGLE("bio_scent_vision", 4500, c_ltblue, 8);
// aquatic
BIO_SINGLE("bio_membrane", 4500, c_blue, 6);
BIO_SINGLE("bio_gills", 4500, c_blue, 6);
// combat augs
BIO_SINGLE("bio_targeting", 6500, c_red, 5);
BIO_SINGLE("bio_ground_sonar", 4500, c_red, 5);
// hazmat
BIO_SINGLE("bio_purifier", 4500, c_ltgreen, 4);
BIO_SINGLE("bio_sunglasses", 4500, c_ltgreen, 4);
BIO_SINGLE("bio_climate", 3500, c_ltgreen, 3);
BIO_SINGLE("bio_heatsink", 3500, c_ltgreen, 3);
BIO_SINGLE("bio_blood_filter", 3500, c_ltgreen, 3);
BIO_SINGLE("bio_geiger", 3500, c_ltgreen, 3);
BIO_SINGLE("bio_radscrubber", 4500, c_ltgreen, 4);
// nutritional
BIO_SINGLE("bio_recycler", 8500, c_green, 6);
BIO_SINGLE("bio_digestion", 5500, c_green, 6);
BIO_SINGLE("bio_evap", 5500, c_green, 4);
BIO_SINGLE("bio_water_extractor", 5500, c_green, 5);
// was: desert survival (all dupes)
// melee:
BIO_SINGLE("bio_shock", 5500, c_red, 5);
BIO_SINGLE("bio_heat_absorb", 5500, c_red, 5);
BIO_SINGLE("bio_claws", 5500, c_red, 5);
BIO_SINGLE("bio_razors", 4500, c_red, 4);
BIO_SINGLE("bio_shockwave", 5500, c_red, 5);
// armor:
BIO_SINGLE("bio_carbon", 7500, c_cyan, 9);
BIO_SINGLE("bio_armor_head", 3500, c_cyan, 5);
BIO_SINGLE("bio_armor_torso", 3500, c_cyan, 4);
BIO_SINGLE("bio_armor_arms", 3500, c_cyan, 3);
BIO_SINGLE("bio_armor_legs", 3500, c_cyan, 3);
BIO_SINGLE("bio_armor_eyes", 5500, c_cyan, 5);
// espionage
BIO_SINGLE("bio_face_mask", 8500, c_magenta, 5);
BIO_SINGLE("bio_scent_mask", 8500, c_magenta, 5);
BIO_SINGLE("bio_cloak", 8500, c_magenta, 5);
BIO_SINGLE("bio_fingerhack", 3500, c_magenta, 2);
BIO_SINGLE("bio_night", 8500, c_magenta, 5);
// defensive
BIO_SINGLE("bio_ads", 9500, c_ltblue, 7);
BIO_SINGLE("bio_ods", 9500, c_ltblue, 7);
BIO_SINGLE("bio_uncanny_dodge", 9500, c_ltblue, 11);
// medical
BIO_SINGLE("bio_nanobots", 9500, c_ltred, 6);
BIO_SINGLE("bio_blood_anal", 3200, c_ltred, 2);
// construction
BIO_SINGLE("bio_resonator", 12000, c_dkgray, 11);
BIO_SINGLE("bio_hydraulics", 4000, c_dkgray, 6);
// super soldier
BIO_SINGLE("bio_time_freeze", 14000, c_white, 11);
BIO_SINGLE("bio_teleport", 7000, c_white, 7);
BIO_SINGLE("bio_probability_travel", 14000, c_white, 11);
BIO_SINGLE("bio_str_enhancer", 8000, c_ltblue, 11);
BIO_SINGLE("bio_dex_enhancer", 8000, c_ltblue, 11);
// ranged combat
BIO_SINGLE("bio_blaster", 2200, c_red, 3);
BIO_SINGLE("bio_laser", 7200, c_red, 5);
BIO_SINGLE("bio_emp", 7200, c_red, 5);
BIO_SINGLE("bio_flashbang", 7200, c_red, 5);
BIO_SINGLE("bio_railgun", 2200, c_red, 3);
BIO_SINGLE("bio_chain_lightning", 2200, c_red, 3);
// power armor
BIO_SINGLE("bio_power_armor_interface", 1200, c_yellow, 1);
BIO_SINGLE("bio_power_armor_interface_mkII", 10000, c_yellow, 8);

SOFTWARE("software_useless", _("misc software"), 300, SW_USELESS, 0, _("\
A miscellaneous piece of hobby software. Probably useless."));

SOFTWARE("software_hacking", _("hackPRO"), 800, SW_HACKING, 2, _("\
A piece of hacking software."));

SOFTWARE("software_medical", _("MediSoft"), 600, SW_MEDICAL, 2, _("\
A piece of medical software."));

SOFTWARE("software_math", _("MatheMAX"), 500, SW_SCIENCE, 3, _("\
A piece of mathematical software."));

SOFTWARE("software_blood_data", _("infection data"), 200, SW_DATA, 5, _("\
Medical data on zombie blood."));

MACGUFFIN("note", _("note"), 0, '?', c_white, "paper", "null", 1, 3, 0, 0, 0,
    true, &iuse::mcg_note, _("\
A hand-written paper note."));

#define STATIONARY(id, name, price, category, description) \
itypes[id] = new it_stationary(id, price, name, description,\
',', c_white, "paper", "null", 0, 3, 0, 0, 0, category)

STATIONARY("flyer", _("flyer"), 1, "flier", _("A scrap of paper."));

// Finally, add all the keys from the map to a vector of all possible items
for(std::map<std::string,itype*>::iterator iter = itypes.begin(); iter != itypes.end(); ++iter){
    if(iter->first == "null" || iter->first == "corpse" || iter->first == "toolset" || iter->first == "fire" || iter->first == "apparatus"){
        pseudo_itype_ids.push_back(iter->first);
    } else {
        standard_itype_ids.push_back(iter->first);
    }
}

std::set<std::string> noammo_flags;
noammo_flags.insert("NO_AMMO");

std::set<std::string> laser_effects;
laser_effects.insert("LASER");
laser_effects.insert("INCENDIARY");
GUN("bio_laser_gun", _("laser finger"),  0,c_magenta, "steel", "plastic",
// SKILL   AMMO              VOL WGT MDG HIT DMG RNG ACC REC DUR BST CLIP REL
 "pistol", "generic_no_ammo", 12,  0,  0,  0,  10, 30,  4,  0, 10,  0,  1, 500,
//    AP
  "", 15, noammo_flags, laser_effects);

std::set<std::string> fusion_effects;
fusion_effects.insert("PLASMA");
fusion_effects.insert("INCENDIARY");
//  NAME  RARE COLOR  MAT1 MAT2
GUN("bio_blaster_gun", _("fusion blaster"),  0,c_magenta, "steel", "plastic",
// SKILL  AMMO             VOL WGT MDG HIT DMG RNG ACC REC DUR BST CLIP REL
 "rifle", "generic_no_ammo", 12,  0,  0,  0, 22, 30,  4,  0, 10,  0,  1, 500,
//    AP
  "", 15, noammo_flags, fusion_effects);

std::set<std::string> lightning_effects;
lightning_effects.insert("LIGHTNING");
lightning_effects.insert("BOUNCE");
//  NAME  RARE COLOR  MAT1 MAT2
GUN("bio_lightning", _("Chain Lightning"),  0,c_magenta, "steel", "plastic",
// SKILL  AMMO              VOL WGT MDG HIT DMG RNG ACC REC DUR BST CLIP REL
 "rifle", "generic_no_ammo", 12,  0,  0,  0,  6,  10,  0,  0, 10,  1, 10, 500,
"", 0, noammo_flags, lightning_effects);
}

std::string ammo_name(ammotype t)
{
    if( t == "700nx")       return _(".700 Nitro Express");
    if( t == "ammo_flint")  return _("flintlock paper pack");
    if( t == "50")          return _(".50 BMG");
    if( t == "nail")        return _("nails");
    if( t == "BB" )         return _("BBs");
    if( t == "bolt" )       return _("bolts");
    if( t == "arrow" )      return _("arrows");
    if( t == "pebble" )     return _("pebbles");
    if( t == "shot" )       return _("shot");
    if( t == "22" )         return _(".22");
    if( t == "9mm" )        return _("9mm");
    if( t == "762x25" )     return _("7.62x25mm");
    if( t == "38" )         return _(".38");
    if( t == "40" )         return _(".40");
    if( t == "44" )         return _(".44");
    if( t == "45" )         return _(".45");
    if( t == "454" )        return _(".454");
    if( t == "500" )        return _(".500");
    if( t == "57" )         return _("5.7mm");
    if( t == "46" )         return _("4.6mm");
    if( t == "762" )        return _("7.62x39mm");
    if( t == "223" )        return _(".223");
    if( t == "3006" )       return _(".30-06");
    if( t == "308" )        return _(".308");
    if( t == "40mm" )       return _("40mm grenade");
    if( t == "66mm" )       return _("High Explosive Anti Tank warhead");
    if( t == "84x246mm" )   return _("84mm recoilless projectile");
    if( t == "m235" )       return _("M235 Incendiary TPA");
    if( t == "gasoline" )   return _("gasoline");
    if( t == "thread" )     return _("thread");
    if( t == "battery" )    return _("batteries");
    if( t == "laser_capacitor")return _("charge");
    if( t == "plutonium" )  return _("plutonium");
    if( t == "muscle" )     return _("muscle");
    if( t == "fusion" )     return _("fusion cell");
    if( t == "12mm" )       return _("12mm slugs");
    if( t == "plasma" )     return _("hydrogen");
    if( t == "water" )      return _("clean water");
    if( t == "8x40mm" )     return _("8x40mm caseless");
    if( t == "20x66mm" )    return _("20x66mm caseless shotgun");
    if( t == "5x50" )       return _("5x50mm flechette");
    if( t == "signal_flare")return _("signal flare");
    if( t == "mininuke_mod")return _("modified mininuke");
    if( t == "charcoal" )   return _("charcoal");
    if( t == "metal_rail" ) return _("ferrous rail projectile");
    if( t == "UPS" )        return _("UPS");
    if( t == "thrown" )     return _("throwing weapon");
    if( t == "ampoule" )    return _("chemical ampoule");
    if( t == "components" ) return _("components");
    if( t == "RPG-7" )      return _("RPG-7");
    if( t == "dart" )       return _("dart");
    if( t == "fishspear" )  return _("speargun spear");
    return "XXX";
}

itype_id default_ammo(ammotype guntype)
{
    if( guntype == "nail" )         return "nail";
    if( guntype == "BB" )           return "bb";
    if( guntype == "bolt" )         return "bolt_wood";
    if( guntype == "arrow" )        return "arrow_wood";
    if( guntype == "pebble" )       return "pebble";
    if( guntype == "shot" )         return "shot_00";
    if( guntype == "22" )           return "22_lr";
    if( guntype == "9mm" )          return "9mm";
    if( guntype == "762x25" )       return "762_25";
    if( guntype == "38" )           return "38_special";
    if( guntype == "40" )           return "10mm";
    if( guntype == "44" )           return "44magnum";
    if( guntype == "45" )           return "45_acp";
    if( guntype == "454" )          return "454_Casull";
    if( guntype == "500" )          return "500_Magnum";
    if( guntype == "57" )           return "57mm";
    if( guntype == "46" )           return "46mm";
    if( guntype == "762" )          return "762_m43";
    if( guntype == "223" )          return "223";
    if( guntype == "308" )          return "308";
    if( guntype == "3006" )         return "270";
    if( guntype == "40mm" )         return "40mm_concussive";
    if( guntype == "66mm" )         return "66mm_HEAT";
    if( guntype == "84x246mm" )     return "84x246mm_he";
    if( guntype == "m235" )         return "m235tpa";
    if( guntype == "battery" )      return "battery";
    if( guntype == "fusion" )       return "laser_pack";
    if( guntype == "12mm" )         return "12mm";
    if( guntype == "plasma" )       return "plasma";
    if( guntype == "plutonium" )    return "plut_cell";
    if( guntype == "gasoline" )     return "gasoline";
    if( guntype == "thread" )       return "thread";
    if( guntype == "water" )        return "water_clean";
    if( guntype == "charcoal"  )    return "charcoal";
    if( guntype == "8x40mm"  )      return "8mm_caseless";
    if( guntype == "20x66mm"  )     return "20x66_shot";
    if( guntype == "5x50"  )        return "5x50dart";
    if( guntype == "signal_flare")  return "signal_flare";
    if( guntype == "mininuke_mod")  return "mininuke_mod";
    if( guntype == "metal_rail"  )  return "rebar_rail";
    if( guntype == "UPS"  )         return "UPS";
    if( guntype == "components"  )  return "components";
    if( guntype == "thrown"  )      return "thrown";
    if( guntype == "ampoule"  )     return "ampoule";
    if( guntype == "50"  )          return "50bmg";
    if( guntype == "fishspear"  )   return "fishspear";
    return "null";
}
