#include "itype.h"
#include "game.h"
#include "setvector.h"
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

// GENERAL GUIDELINES
// When adding a new item, you MUST REMEMBER to insert it in the itype_id enum
//  at the top of itype.h!
//  Additionally, you should check mapitemsdef.cpp and insert the new item in
//  any appropriate lists.
void game::init_itypes ()
{
// First, the null object.  NOT REALLY AN OBJECT AT ALL.  More of a concept.
 itypes["null"]=
  new itype("null", 0, "none", "", '#', c_white, "null", "null", PNULL, 0, 0, 0, 0, 0, 0);
// Corpse - a special item
 itypes["corpse"]=
  new itype("corpse", 0, "corpse", "A dead body.", '%', c_white, "null", "null", PNULL, 0, 0,
            0, 0, 1, 0);
 itypes["corpse"]->item_tags.insert("NO_UNLOAD");
// This must -always- be set, or bad mojo in map::drawsq and whereever we check 'typeId() == "corpse" instead of 'corpse != NULL' ....
 itypes["corpse"]->corpse=this->mtypes[mon_null];
// Fire - only appears in crafting recipes
 itypes["fire"]=
  new itype("fire", 0, "nearby fire",
            "Some fire - if you are reading this it's a bug! (itypdef:fire)",
            '$', c_red, "null", "null", PNULL, 0, 0, 0, 0, 0, 0);
// Integrated toolset - ditto
 itypes["toolset"]=
  new itype("toolset", 0, "integrated toolset",
            "A fake item. If you are reading this it's a bug! (itypdef:toolset)",
            '$', c_red, "null", "null", PNULL, 0, 0, 0, 0, 0, 0);
// For smoking crack or meth
 itypes["apparatus"]=
  new itype("apparatus", 0, "something to smoke that from, and a lighter",
            "A fake item. If you are reading this it's a bug! (itypdef:apparatus)",
            '$', c_red, "null", "null", PNULL, 0, 0, 0, 0, 0, 0);

#define VAR_VEH_PART(id, name,price,sym,color,mat1,mat2,volume,wgt,dam,cut,to_hit,\
              flags, bigmin, bigmax, bigaspect, des)\
itypes[id]=new it_var_veh_part(id,price,name,des,sym,\
color,mat1,mat2,volume,wgt,dam,cut,to_hit,flags, bigmin, bigmax, bigaspect)

//"wheel", "wheel_wide", "wheel_bicycle", "wheel_motorbike", "wheel_small",
//           NAME     RAR PRC  SYM COLOR        MAT1    MAT2
VAR_VEH_PART("wheel", _("wheel"), 100, ']', c_dkgray,  "steel",   "plastic",
//  VOL WGT DAM CUT HIT FLAGS BIGNESS_MIN BIGNESS_MAX  BIGNESS_ASPECT
    40, 8845, 12,  0,  -1, 0,       13,         20,  BIGNESS_WHEEL_DIAMETER,  _("\
A car wheel"));
//           NAME         RAR PRC  SYM COLOR        MAT1    MAT2
VAR_VEH_PART("wheel_wide", _("wide wheel"), 340, ']', c_dkgray,  "steel",   "plastic",
//  VOL WGT  DAM CUT HIT FLAGS BIGNESS_MIN BIGNESS_MAX   ASPECT
    70,22600, 17,  0,  -1, 0,       17,         36,  BIGNESS_WHEEL_DIAMETER,  _("\
A wide wheel. \\o/ This wide."));
//           NAME            RAR  PRC  SYM COLOR        MAT1    MAT2
VAR_VEH_PART("wheel_bicycle", _("bicycle wheel"), 40,  ']', c_dkgray,  "steel",   "plastic",
//  VOL WGT  DAM CUT HIT FLAGS BIGNESS_MIN BIGNESS_MAX    ASPECT
    28,1500,  8,  0,  -1, 0,       9,         18,  BIGNESS_WHEEL_DIAMETER,  _("\
A bicycle wheel"));
//           NAME              RAR  PRC   SYM COLOR        MAT1    MAT2
VAR_VEH_PART("wheel_motorbike", _("motorbike wheel"), 140,  ']', c_dkgray,  "steel",   "plastic",
//  VOL WGT  DAM CUT HIT FLAGS BIGNESS_MIN BIGNESS_MAX    ASPECT
    33,5443,  10,  0,  -1, 0,       9,         14,  BIGNESS_WHEEL_DIAMETER,  _("\
A motorbike wheel"));
//           NAME              RAR  PRC   SYM COLOR        MAT1    MAT2
VAR_VEH_PART("wheel_small", _("small wheel"),140,  ']', c_dkgray,  "steel",   "plastic",
//  VOL WGT  DAM CUT HIT FLAGS BIGNESS_MIN BIGNESS_MAX    ASPECT
    9, 2722,  10,  0,  -1, 0,       6,         14,   BIGNESS_WHEEL_DIAMETER,  _("\
A pretty small wheel. Probably from one of those segway things.\
It is not very menacing."));

//                                 NAME           RAR PRC SYM COLOR        MAT1    MAT2
VAR_VEH_PART("1cyl_combustion", _("1-cylinder engine"), 100, ':', c_ltcyan,  "iron",   "null",
//  VOL WGT DAM CUT HIT FLAGS 0BIGNESS_MIN BIGNESS_MAX   ASPECT
    6, 20000,  4,  0,  -1, 0,       28,         75,   BIGNESS_ENGINE_DISPLACEMENT, _("\
A single-cylinder 4-stroke combustion engine."));

//                              NAME           RAR PRC SYM COLOR        MAT1    MAT2
VAR_VEH_PART("v2_combustion", _("V-twin engine"), 100, ':', c_ltcyan,  "iron",   "null",
//  VOL WGT DAM CUT HIT FLAGS BIGNESS_MIN BIGNESS_MAX ASPECT
    6, 45000,  4,  0,  -1, 0,       65,        260, BIGNESS_ENGINE_DISPLACEMENT, _("\
A 2-cylinder 4-stroke combustion engine."));

//                                NAME           RAR PRC SYM COLOR        MAT1    MAT2
VAR_VEH_PART("i4_combustion", _("Inline-4 engine"), 150, ':', c_ltcyan,  "iron",   "null",
//  VOL WGT DAM CUT HIT FLAGS BIGNESS_MIN BIGNESS_MAX ASPECT
    6, 70000,  8,  0,  -2, 0,       220,       350, BIGNESS_ENGINE_DISPLACEMENT, _("\
A small, yet powerful 4-cylinder combustion engine."));

//                          NAME           RAR PRC SYM COLOR        MAT1    MAT2
VAR_VEH_PART("v6_combustion", _("V6 engine"), 180, ':', c_ltcyan,  "iron",   "null",
//  VOL WGT DAM CUT HIT FLAGS BIGNESS_MIN BIGNESS_MAX ASPECT
    14,100000,  12,  0,  -3, 0,    250,        520, BIGNESS_ENGINE_DISPLACEMENT, _("\
A powerful 6-cylinder combustion engine."));

//                          NAME           RAR PRC SYM COLOR        MAT1    MAT2
VAR_VEH_PART("v8_combustion", _("V8 engine"), 250, ':', c_ltcyan,  "iron",   "null",
//  VOL WGT DAM CUT HIT FLAGS BIGNESS_MIN BIGNESS_MAX ASPECT
    25,144000,  15,  0,  -5, 0,    380,     700, BIGNESS_ENGINE_DISPLACEMENT, _("\
A large and very powerful 8-cylinder combustion engine."));

// GUNS
// ammo_type matches one of the ammo_types above.
// dmg is ADDED to the damage of the corresponding ammo.  +/-, should be small.
// aim affects chances of hitting; low for handguns, hi for rifles, etc, small.
// Durability is rated 1-10; 10 near perfect, 1 it breaks every few shots
// Burst is the # of rounds fired, 0 if no burst ability.
// clip is how many shots we get before reloading.

#define GUN(id,name,price,color,mat1,mat2,skill,ammo,volume,wgt,melee_dam,\
to_hit,dmg,range,dispersion,recoil,durability,burst,clip,reload_time,des) \
itypes[id]=new it_gun(id,price,name,des,'(',\
color,mat1,mat2,volume,wgt,melee_dam,0,to_hit,skill,ammo,dmg,range,dispersion,\
recoil,durability,burst,clip,reload_time)

/* TOOLS
 * MAX is the maximum number of charges help.
 * DEF is the default number of charges--items will be generated with this
 *  many charges.
 * USE is how many charges are used up when 'a'pplying the object.
 * SEC is how many turns will pass before a charge is drained if the item is
 *  active; generally only used in the "<whatever> (on)" forms
 * FUEL is the type of charge the tool uses; set to "NULL" if the item is
 *  unable to be recharged.
 * REVERT is the item type that the tool will revert to once its charges are
 *  drained
 * FUNCTION is a function called when the tool is 'a'pplied, or called once per
 *  turn if the tool is active.  The same function can be used for both.  See
 *  iuse.h and iuse.cpp for functions.
 */
#define TOOL(id, name,price,sym,color,mat1,mat2,volume,wgt,melee_dam,\
melee_cut,to_hit,max_charge,def_charge,charge_per_use,charge_per_sec,fuel,\
revert,func,des) \
itypes[id]=new it_tool(id,price,name,des,sym,\
color,mat1,mat2,SOLID,volume,wgt,melee_dam,melee_cut,to_hit,max_charge,\
def_charge,charge_per_use,charge_per_sec,fuel,revert,func)

TOOL("jack", _("jack"), 86, ';', c_ltgray,	"iron",	"null",
//	VOL WGT DAM CUT HIT FLAGS
	 5,11974, 11,  0,  2, 0, 0, 0, 0, "NULL", "null", &iuse::none, _("\
A common hydraulic jack, used when changing tires."));

// BIONICS
// These are the modules used to install new bionics in the player.  They're
// very simple and straightforward; a difficulty, followed by a NULL-terminated
// list of options.
#define BIO(id, name, price, color, difficulty, des) \
itypes[id]=new it_bionic(id, price,name,des,':',\
color, "steel", "plastic", 10,2041, 8, 0, 0, difficulty)

#define BIO_SINGLE(id,price,color,difficulty) \
     BIO(id, std::string("CBM: ")+bionics[id]->name, price,color,difficulty, \
           bionics[id]->description) \

//  Name                     RAR PRICE    COLOR   DIFFICULTY
BIO("bio_power_storage", _("CBM: Power Storage"), 3800,	c_green,	 1, _("\
Compact Bionics Module that upgrades your power capacity by 4 units. Having\n\
at least one of these is a prerequisite to using powered bionics. You will\n\
also need a power supply, found in another CBM.")); // This is a special case, which increases power capacity by 4

 BIO("bio_power_storage_mkII", _("CBM: Power Storage Mk. II"), 10000, c_green, 1, _("\
Compact Bionics Module developed at DoubleTech Industries as a replacement\n\
for the highly sucessful CBM: Power Storage. Increases you power capacity\n\
by 10 units.")); // This is another special case, increases power capacity by 10 units

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
BIO_SINGLE("bio_climate", 3500, c_ltgreen, 3);
BIO_SINGLE("bio_heatsink", 3500, c_ltgreen, 3);
BIO_SINGLE("bio_blood_filter", 3500, c_ltgreen, 3);
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
BIO_SINGLE("bio_shockwave", 5500, c_red, 5);
// armor:
BIO_SINGLE("bio_carbon", 7500, c_cyan, 9);
BIO_SINGLE("bio_armor_head", 3500, c_cyan, 5);
BIO_SINGLE("bio_armor_torso", 3500, c_cyan, 4);
BIO_SINGLE("bio_armor_arms", 3500, c_cyan, 3);
BIO_SINGLE("bio_armor_legs", 3500, c_cyan, 3);
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

//  NAME		RARE	COLOR		MAT1	MAT2
GUN("bio_blaster_gun", _("fusion blaster"),	 0,c_magenta,	"steel",	"plastic",
//	SKILL		AMMO	   VOL WGT MDG HIT DMG RNG ACC REC DUR BST CLIP REL
	"rifle",	"fusion", 12,  0,  0,  0,  0,  0,  4,  0, 10,  0,  1, 500,
"");

//  NAME		RARE	COLOR		MAT1	MAT2
GUN("bio_lightning", _("Chain Lightning"),	 0,c_magenta,	"steel",	"plastic",
//	SKILL		AMMO	   VOL WGT MDG HIT DMG RNG ACC REC DUR BST CLIP REL
	"rifle",	"fusion", 12,  0,  0,  0,  0,  0,  4,  0, 10,  1,  10, 500,
"");


// Unarmed Styles
// TODO: refactor handling of styles see #1771
#define STYLE(id, name, dam, description, ...) \
itypes[id]=new it_style(id, 0, name, description, '$', \
                              c_white, "null", "null", 0, 0, dam, 0, 0); \
 setvector(&((static_cast<it_style*>(itypes[id])))->moves, __VA_ARGS__, NULL); \
itypes[id]->item_tags.insert("UNARMED_WEAPON"); \
martial_arts_itype_ids.push_back(id)

STYLE("style_karate", _("karate"), 2, _("\
Karate is a popular martial art, originating from Japan. It focuses on\n\
rapid, precise attacks, blocks, and fluid movement. A successful hit allows\n\
you an extra dodge and two extra blocks on the following round."),

"quickly punch", _("%1$s quickly punch %4$s"), _("%1$s quickly punches %4$s"), TEC_RAPID, 0,
"block", _("%1$s block %4$s"), _("%1$s blocks %4$s"), TEC_BLOCK, 2,
"karate chop", _("%1$s karate chop %4$s"), _("%1$s karate chops %4$s"), TEC_PRECISE, 4
);

STYLE("style_aikido", _("aikido"), 0, _("\
Aikido is a Japanese martial art focused on self-defense, while minimizing\n\
injury to the attacker. It uses defense throws and disarms. Damage done\n\
while using this technique is halved, but pain inflicted is doubled."),

"feint at", _("%1$s feint at %4$s"), _("%1$s feints at %4$s"), TEC_FEINT, 2,
"throw", _("%1$s throw %4$s"), _("%1$s throws %4$s"), TEC_DEF_THROW, 2,
"disarm", _("%1$s disarm %4$s"), _("%1$s disarms %4$s"), TEC_DISARM, 3,
"disarm", _("%1$s disarm %4$s"), _("%1$s disarms %4$s"), TEC_DEF_DISARM, 4
);

STYLE("style_judo", _("judo"), 0, _("\
Judo is a martial art that focuses on grabs and throws, both defensive and\n\
offensive. It also focuses on recovering from throws; while using judo, you\n\
will not lose any turns to being thrown or knocked down."),

"grab", _("%1$s grab %4$s"), _("%1$s grabs %4$s"), TEC_GRAB, 2,
"throw", _("%1$s throw %4$s"), _("%1$s throws %4$s"), TEC_THROW, 3,
"throw", _("%1$s throw %4$s"), _("%1$s throws %4$s"), TEC_DEF_THROW, 4
);

STYLE("style_tai_chi", _("tai chi"), 0, _("\
Though tai chi is often seen as a form of mental and physical exercise, it is\n\
a legitimate martial art, focused on self-defense. Its ability to absorb the\n\
force of an attack makes your Perception decrease damage further on a block."),

"block", _("%1$s block %4$s"), _("%1$s blocks %4$s"), TEC_BLOCK, 1,
"disarm", _("%1$s disarm %4$s"), _("%1$s disarms %4$s"), TEC_DEF_DISARM, 3,
"strike", _("%1$s strike %4$s"), _("%1$s strikes %4$s"), TEC_PRECISE, 4
);

STYLE("style_capoeira", _("capoeira"), 1, _("\
A dance-like style with its roots in Brazilian slavery, capoeira is focused\n\
on fluid movement and sweeping kicks. Moving a tile will boost attack and\n\
dodge; attacking boosts dodge, and dodging boosts attack."),

"bluff", _("%1$s bluff %4$s"), _("%1$s bluffs %4$s"), TEC_FEINT, 1,
"low kick", _("%1$s low kick %4$s"), _("%1$s low kicks %4$s"), TEC_SWEEP, 3,
"spin and hit", _("%1$s spin and hit %4$s"), _("%1$s spins and hits %4$s"), TEC_COUNTER, 4,
"spin-kick", _("%1$s spin-kick %4$s"), _("%1$s spin-kicks %4$s"), TEC_WIDE, 5
);

STYLE("style_krav_maga", _("krav maga"), 4, _("\
Originating in Israel, Krav Maga is based on taking down an enemy quickly and\n\
effectively. It focuses on applicable attacks rather than showy or complex\n\
moves. Popular among police and armed forces everywhere."),

"quickly punch", _("%1$s quickly punch %4$s"), _("%1$s quickly punches %4$s"), TEC_RAPID, 2,
"block", _("%1$s block %4$s"), _("%1$s blocks %4$s"), TEC_BLOCK, 2,
"feint at", _("%1$s feint at %4$s"), _("%1$s feints at %4$s"), TEC_FEINT, 3,
"jab", _("%1$s jab %4$s"), _("%1$s jabs %4$s"), TEC_PRECISE, 3,
"disarm", _("%1$s disarm %4$s"), _("%1$s disarms %4$s"), TEC_DISARM, 3,
"block", _("%1$s block %4$s"), _("%1$s blocks %4$s"), TEC_BLOCK_LEGS, 4,
"counter-attack", _("%1$s counter-attack %4$s"), _("%1$s counter-attacks %4$s"), TEC_COUNTER, 4,
"disarm", _("%1$s disarm %4$s"), _("%1$s disarms %4$s"), TEC_DEF_DISARM, 4,
"", "", "", TEC_BREAK, 4,
"grab", _("%1$s grab %4$s"), _("%1$s grabs %4$s"), TEC_GRAB, 5
);

STYLE("style_muay_thai", _("muay thai"), 4, _("\
Also referred to as the \"Art of 8 Limbs,\" Muay Thai is a popular fighting\n\
technique from Thailand that uses powerful strikes. It does extra damage\n\
against large or strong opponents."),

"slap", _("%1$s slap %4$s"), _("%1$s slaps %4$s"), TEC_RAPID, 2,
"block", _("%1$s block %4$s"), _("%1$s blocks %4$s"), TEC_BLOCK, 3,
"block", _("%1$s block %4$s"), _("%1$s blocks %4$s"), TEC_BLOCK_LEGS, 4,
"power-kick", _("%1$s power-kick %4$s"), _("%1$s power-kicks %4$s"), TEC_BRUTAL, 4,
"counter-attack", _("%1$s counter-attack %4$s"), _("%1$s counter-attacks %4$s"), TEC_COUNTER, 5
);

STYLE("style_ninjutsu", _("ninjutsu"), 1, _("\
Ninjutsu is a martial art and set of tactics used by ninja in feudal Japan.\n\
It focuses on rapid, precise, silent strikes. Ninjutsu is entirely silent.\n\
It also provides small combat bonuses the turn after moving a tile."),

"quickly punch", _("%1$s quickly punch %4$s"), _("%1$s quickly punches %4$s"), TEC_RAPID, 3,
"jab", _("%1$s jab %4$s"), _("%1$s jabs %4$s"), TEC_PRECISE, 4,
"block", _("%1$s block %4$s"), _("%1$s blocks %4$s"), TEC_BLOCK, 4
);

STYLE("style_taekwondo", _("taekwondo"), 2, _("\
Taekwondo is the national sport of Korea, and was used by the South Korean\n\
army in the 20th century. Focused on kicks and punches, it also includes\n\
strength training; your blocks absorb extra damage the stronger you are."),

"block", _("%1$s block %4$s"), _("%1$s blocks %4$s"), TEC_BLOCK, 2,
"block", _("%1$s block %4$s"), _("%1$s blocks %4$s"), TEC_BLOCK_LEGS, 3,
"jab", _("%1$s jab %4$s"), _("%1$s jabs %4$s"), TEC_PRECISE, 4,
"brutally kick", _("%1$s brutally kick %4$s"), _("%1$s brutally kicks %4$s"), TEC_BRUTAL, 4,
"spin-kick", _("%1$s spin-kick %4$s"), _("%1$s spin-kicks %4$s"), TEC_SWEEP, 5
);

STYLE("style_tiger", _("tiger style"), 4, _("\
One of the five Shaolin animal styles. Tiger style focuses on relentless\n\
attacks above all else. Strength, not Dexterity, is used to determine hits;\n\
you also receive an accumulating bonus for several turns of sustained attack."),

"grab", _("%1$s grab %4$s"), _("%1$s grabs %4$s"), TEC_GRAB, 4
);

STYLE("style_crane", _("crane style"), 0, _("\
One of the five Shaolin animal styles. Crane style uses intricate hand\n\
techniques and jumping dodges. Dexterity, not Strength, is used to determine\n\
damage; you also receive a dodge bonus the turn after moving a tile."),

"feint at", _("%1$s feint at %4$s"), _("%1$s feints at %4$s"), TEC_FEINT, 2,
"block", _("%1$s block %4$s"), _("%1$s blocks %4$s"), TEC_BLOCK, 3,
"", "", "", TEC_BREAK, 3,
"hand-peck", _("%1$s hand-peck %4$s"), _("%1$s hand-pecks %4$s"), TEC_PRECISE, 4
);

STYLE("style_leopard", _("leopard style"), 3, _("\
One of the five Shaolin animal styles. Leopard style focuses on rapid,\n\
strategic strikes. Your Perception and Intelligence boost your accuracy, and\n\
moving a single tile provides an increased boost for one turn."),

"swiftly jab", _("%1$s swiftly jab %4$s"), _("%1$s swiftly jabs %4$s"), TEC_RAPID, 2,
"counter-attack", _("%1$s counter-attack %4$s"), _("%1$s counter-attacks %4$s"), TEC_COUNTER, 4,
"leopard fist", _("%1$s strike %4$s with leopard fist"), _("%1$s strikes %4$s with leopard fist"), TEC_PRECISE, 5
);

STYLE("style_snake", _("snake style"), 1, _("\
One of the five Shaolin animal styles. Snake style uses sinuous movement and\n\
precision strikes. Perception increases your chance to hit as well as the\n\
damage you deal."),

"swiftly jab", _("%1$s swiftly jab %4$s"), _("%1$s swiftly jabs %4$s"), TEC_RAPID, 2,
"feint at", _("%1$s feint at %4$s"), _("%1$s feints at %4$s"), TEC_FEINT, 3,
"snakebite", _("%1$s snakebite %4$s"), _("%1$s snakebites %4$s"), TEC_PRECISE, 4,
"writhe free from", _("%1$s writhe free from %4$s"), _("%1$s writhes free from %4$s"), TEC_BREAK, 4
);

STYLE("style_dragon", _("dragon style"), 2, _("\
One of the five Shaolin animal styles. Dragon style uses fluid movements and\n\
hard strikes. Intelligence increases your chance to hit as well as the\n\
damage you deal. Moving a tile will boost damage further for one turn."),

"", "", "", TEC_BLOCK, 2,
"grab", _("%1$s grab %4$s"), _("%1$s grabs %4$s"), TEC_GRAB, 4,
"counter-attack", _("%1$s counter-attack %4$s"), _("%1$s counter-attacks %4$s"), TEC_COUNTER, 4,
"spin-kick", _("%1$s spin-kick %4$s"), _("%1$s spin-kicks %4$s"), TEC_SWEEP, 5,
"dragon strike", _("%1$s use dragon strike on %4$s"), _("%1$s uses dragon strike on %4$s"), TEC_BRUTAL, 6
);

STYLE("style_centipede", _("centipede style"), 0, _("\
One of the Five Deadly Venoms. Centipede style uses an onslaught of rapid\n\
strikes. Every strike you make reduces the movement cost of attacking by 4;\n\
this is cumulative, but is reset entirely if you are hit even once."),

"swiftly hit", _("%1$s swiftly hit %4$s"), _("%1$s swiftly hits %4$s"), TEC_RAPID, 2,
"block", _("%1$s block %4$s"), _("%1$s blocks %4$s"), TEC_BLOCK, 3
);

STYLE("style_venom_snake", _("viper style"), 2, _("\
One of the Five Deadly Venoms. Viper Style has a unique three-hit combo; if\n\
you score a critical hit, it is initiated. The second hit uses a coned hand\n\
to deal piercing damage, and the 3rd uses both hands in a devastating strike."),

"", "", "", TEC_RAPID, 3,
"feint at", _("%1$s feint at %4$s"), _("%1$s feints at %4$s"), TEC_FEINT, 3,
"writhe free from", _("%1$s writhe free from %4$s"), _("%1$s writhes free from %4$s"), TEC_BREAK, 4
);

STYLE("style_scorpion", _("scorpion style"), 3, _("\
One of the Five Deadly Venoms. Scorpion Style is a mysterious art that focuses\n\
on utilizing pincer-like fists and a stinger-like kick. Critical hits will do\n\
massive damage, knocking your target far back."),

"block", _("%1$s block %4$s"), _("%1$s blocks %4$s"), TEC_BLOCK, 3,
"pincer fist", _("%1$s strike %4$s with spincer fist"), _("%1$s strikes %4$s with spincer fist"), TEC_PRECISE, 4
);

STYLE("style_lizard", _("lizard style"), 1, _("\
One of the Five Deadly Venoms. Lizard Style focuses on using walls to one's\n\
advantage. Moving alongside a wall will make you run up along it, giving you\n\
a large to-hit bonus. Standing by a wall allows you to use it to boost dodge."),

"block", _("%1$s block %4$s"), _("%1$s blocks %4$s"), TEC_BLOCK, 2,
"counter-attack", _("%1$s counter-attack %4$s"), _("%1$s counter-attacks %4$s"), TEC_COUNTER, 4
);

STYLE("style_toad", _("toad style"), 0, _("\
One of the Five Deadly Venoms. Immensely powerful, and immune to nearly any\n\
weapon. You may meditate by pausing for a turn; this will give you temporary\n\
armor, proportional to your Intelligence and Perception."),

"block", _("%1$s block %4$s"), _("%1$s blocks %4$s"), TEC_BLOCK, 3,
"grab", _("%1$s grab %4$s"), _("%1$s grabs %4$s"), TEC_GRAB, 4
);

STYLE("style_zui_quan", _("zui quan"), 1, _("\
Also known as \"drunken boxing,\" Zui Quan imitates the movement of a drunk\n\
to confuse the enemy. The turn after you attack, you may dodge any number of\n\
attacks with no penalty."),

"stumble and leer at", _("%1$s stumble and leer at %4$s"), _("%1$s stumbles and leers at %4$s"), TEC_FEINT, 3,
"counter-attack", _("%1$s counter-attack %4$s"), _("%1$s counter-attacks %4$s"), TEC_COUNTER, 4
);

}

std::string ammo_name(ammotype t)
{
    if( t == "nail")   return _("nails");
    if( t == "BB" )	   return _("BBs");
    if( t == "bolt" )	 return _("bolts");
    if( t == "arrow" ) return _("arrows");
    if( t == "pebble" )return _("pebbles");
    if( t == "shot" )	 return _("shot");
    if( t == "22" )	   return _(".22");
    if( t == "9mm" )	  return _("9mm");
    if( t == "762x25" )return _("7.62x25mm");
    if( t == "38" )	   return _(".38");
    if( t == "40" )	   return _(".40");
    if( t == "44" )	   return _(".44");
    if( t == "45" )	   return _(".45");
    if( t == "454" )	  return _(".454");
    if( t == "500" )	  return _(".500");
    if( t == "57" )	   return _("5.7mm");
    if( t == "46" )	   return _("4.6mm");
    if( t == "762" )	  return _("7.62x39mm");
    if( t == "223" )	  return _(".223");
    if( t == "3006" )  return _(".30-06");
    if( t == "308" )	  return _(".308");
    if( t == "40mm" )  return _("40mm grenade");
    if( t == "66mm" )  return _("High Explosive Anti Tank Warhead");
    if( t == "gasoline" )	  return _("gasoline");
    if( t == "THREAD" )return _("thread");
    if( t == "battery" )  return _("batteries");
    if( t == "plutonium" )  return _("plutonium");
    if( t == "muscle" )return _("Muscle");
    if( t == "fusion" )return _("fusion cell");
    if( t == "12mm" )  return _("12mm slugs");
    if( t == "plasma" )return _("hydrogen");
    if( t == "water"  )return _("clean water");
    if( t == "8x40mm"  )return _("8x40mm caseless");
    return "XXX";
}

itype_id default_ammo(ammotype guntype)
{
    if( guntype == "nail" )	 return "nail";
    if( guntype == "BB" )	   return "bb";
    if( guntype == "bolt" )	 return "bolt_wood";
    if( guntype == "arrow" ) return "arrow_wood";
    if( guntype == "pebble" )return "pebble";
    if( guntype == "shot" )	 return "shot_00";
    if( guntype == "22" )	   return "22_lr";
    if( guntype == "9mm" )	  return "9mm";
    if( guntype == "762x25" )return "762_25";
    if( guntype == "38" )	   return "38_special";
    if( guntype == "40" )	   return "10mm";
    if( guntype == "44" )	   return "44magnum";
    if( guntype == "45" )	   return "45_acp";
    if( guntype == "454" )   return "454_Casull";
    if( guntype == "500" )   return "500_Magnum";
    if( guntype == "57" )	   return "57mm";
    if( guntype == "46" )	   return "46mm";
    if( guntype == "762" )	  return "762_m43";
    if( guntype == "223" )	  return "223";
    if( guntype == "308" )	  return "308";
    if( guntype == "3006" )	 return "270";
    if( guntype == "40mm" )  return "40mm_concussive";
    if( guntype == "66mm" )  return "66mm_HEAT";
    if( guntype == "battery" )	 return "battery";
    if( guntype == "fusion" )return "laser_pack";
    if( guntype == "12mm" )  return "12mm";
    if( guntype == "plasma" )return "plasma";
    if( guntype == "plutonium" )	 return "plut_cell";
    if( guntype == "gasoline" )	  return "gasoline";
    if( guntype == "THREAD" )return "thread";
    if( guntype == "water" ) return "water_clean";
    if( guntype == "8x40mm"  )return "8x40mm caseless";
    return "null";
}
