#ifndef _ARTIFACTDATA_H_
#define _ARTIFACTDATA_H_

#include <vector>
#include "artifact.h"
#include "itype.h"

int passive_effect_cost[NUM_AEPS] = {
0,	// AEP_NULL

3,	// AEP_STR_UP
3,	// AEP_DEX_UP
3,	// AEP_PER_UP
3,	// AEP_INT_UP
5,	// AEP_ALL_UP
4,	// AEP_SPEED_UP
2,	// AEP_IODINE
4,	// AEP_SNAKES
7,	// AEP_INVISIBLE
5,	// AEP_CLAIRVOYANCE
2,	// AEP_STEALTH
2,	// AEP_EXTINGUISH
1,	// AEP_GLOW
1,	// AEP_PSYSHIELD
3,	// AEP_RESIST_ELECTRICITY
3,	// AEP_CARRY_MORE
5,	// AEP_SAP_LIFE

0,	// AEP_SPLIT

-2,	// AEP_HUNGER
-2,	// AEP_THIRST
-1,	// AEP_SMOKE
-5,	// AEP_EVIL
-3,	// AEP_SCHIZO
-5,	// AEP_RADIOACTIVE
-3,	// AEP_MUTAGENIC
-5,	// AEP_ATTENTION
-2,	// AEP_STR_DOWN
-2,	// AEP_DEX_DOWN
-2,	// AEP_PER_DOWN
-2,	// AEP_INT_DOWN
-5,	// AEP_ALL_DOWN
-4,	// AEP_SPEED_DOWN
-5,	// AEP_FORCE_TELEPORT
-3,	// AEP_MOVEMENT_NOISE
-2,	// AEP_BAD_WEATHER
-1	// AEP_SICK
};

int active_effect_cost[NUM_AEAS] = {
 0, // AEA_NULL

 2, // AEA_STORM
 4, // AEA_FIREBALL
 5, // AEA_ADRENALINE
 4, // AEA_MAP
 0, // AEA_BLOOD
 0, // AEA_FATIGUE
 4, // AEA_ACIDBALL
 5, // AEA_PULSE
 4, // AEA_HEAL
 3, // AEA_CONFUSED
 3, // AEA_ENTRANCE
 3, // AEA_BUGS
 5, // AEA_TELEPORT
 1, // AEA_LIGHT
 4, // AEA_GROWTH
 6, // AEA_HURTALL

 0, // AEA_SPLIT

-3, // AEA_RADIATION
-2, // AEA_PAIN
-3, // AEA_MUTATE
-2, // AEA_PARALYZE
-3, // AEA_FIRESTORM
-6, // AEA_ATTENTION
-4, // AEA_TELEGLOW
-2, // AEA_NOISE
-2, // AEA_SCREAM
-3, // AEA_DIM
-4, // AEA_FLASH
-2, // AEA_VOMIT
-5  // AEA_SHADOWS
};

struct artifact_shape_datum
{
 std::string name;
 std::string desc;
 int volume_min, volume_max;
 int weight_min, weight_max;
};

artifact_shape_datum artifact_shape_data[ARTSHAPE_MAX] = {
{"BUG", "BUG", 0, 0, 0, 0},
{"sphere", "smooth sphere", 2, 4, 0, 10},
{"rod", "tapered rod", 1, 7, 1, 7},
{"teardrop", "teardrop-shaped stone", 2, 6, 0, 8},
{"lamp", "hollow, transparent cube", 4, 9, 0, 3},
{"snake", "winding, flexible rod", 0, 8, 0, 8},
{"disc", "smooth disc", 4, 6, 2, 4},
{"beads", "string of beads", 3, 7, 0, 6},
{"napkin", "very thin sheet", 0, 3, 0, 3},
{"urchin", "spiked sphere", 3, 5, 2, 6},
{"jelly", "malleable blob", 2, 8, 2, 4},
{"spiral", "spiraling rod", 5, 6, 2, 3},
{"pin", "pointed rod", 1, 5, 1, 9},
{"tube", "hollow tube", 2, 5, 3, 6},
{"pyramid", "regular tetrahedron", 3, 7, 2, 4},
{"crystal", "translucent crystal", 1, 6, 2, 7},
{"knot", "twisted, knotted cord", 2, 6, 1, 7},
{"crescent", "crescent-shaped stone", 2, 6, 2, 6}
};

struct artifact_property_datum
{
 std::string name;
 std::string desc;
 art_effect_passive passive_good[4];
 art_effect_passive passive_bad[4];
 art_effect_active active_good[4];
 art_effect_active active_bad[4];
};

artifact_property_datum artifact_property_data[ARTPROP_MAX] = {
{"BUG", "BUG", 
 {AEP_NULL, AEP_NULL, AEP_NULL, AEP_NULL},
 {AEP_NULL, AEP_NULL, AEP_NULL, AEP_NULL},
 {AEA_NULL, AEA_NULL, AEA_NULL, AEA_NULL},
 {AEA_NULL, AEA_NULL, AEA_NULL, AEA_NULL}
},
{"wriggling", "is constantly wriggling", 
 {AEP_SPEED_UP, AEP_SNAKES, AEP_NULL, AEP_NULL},
 {AEP_DEX_DOWN, AEP_FORCE_TELEPORT, AEP_SICK, AEP_NULL},
 {AEA_TELEPORT, AEA_ADRENALINE, AEA_NULL, AEA_NULL},
 {AEA_MUTATE, AEA_ATTENTION, AEA_VOMIT, AEA_NULL}
},
{"glowing", "glows faintly", 
 {AEP_INT_UP, AEP_GLOW, AEP_CLAIRVOYANCE, AEP_NULL},
 {AEP_RADIOACTIVE, AEP_MUTAGENIC, AEP_ATTENTION, AEP_NULL},
 {AEA_LIGHT, AEA_LIGHT, AEA_LIGHT, AEA_NULL},
 {AEA_ATTENTION, AEA_TELEGLOW, AEA_FLASH, AEA_SHADOWS}
},
{"humming", "hums very quietly", 
 {AEP_ALL_UP, AEP_PSYSHIELD, AEP_NULL, AEP_NULL},
 {AEP_SCHIZO, AEP_PER_DOWN, AEP_INT_DOWN, AEP_NULL},
 {AEA_PULSE, AEA_ENTRANCE, AEA_NULL, AEA_NULL},
 {AEA_NOISE, AEA_NOISE, AEA_SCREAM, AEA_NULL}
},
{"moving", "shifts from side to side slowly", 
 {AEP_STR_UP, AEP_DEX_UP, AEP_SPEED_UP, AEP_NULL},
 {AEP_HUNGER, AEP_PER_DOWN, AEP_FORCE_TELEPORT, AEP_NULL},
 {AEA_TELEPORT, AEA_TELEPORT, AEA_MAP, AEA_NULL},
 {AEA_PARALYZE, AEA_VOMIT, AEA_VOMIT, AEA_NULL}
},
{"whispering", "makes very faint whispering sounds", 
 {AEP_CLAIRVOYANCE, AEP_EXTINGUISH, AEP_STEALTH, AEP_NULL},
 {AEP_EVIL, AEP_SCHIZO, AEP_ATTENTION, AEP_NULL},
 {AEA_FATIGUE, AEA_ENTRANCE, AEA_ENTRANCE, AEA_NULL},
 {AEA_ATTENTION, AEA_SCREAM, AEA_SCREAM, AEA_SHADOWS}
},
{"breathing",
 "shrinks and grows very slightly with a regular pulse, as if breathing",
 {AEP_SAP_LIFE, AEP_ALL_UP, AEP_SPEED_UP, AEP_CARRY_MORE},
 {AEP_HUNGER, AEP_THIRST, AEP_SICK, AEP_BAD_WEATHER},
 {AEA_ADRENALINE, AEA_HEAL, AEA_ENTRANCE, AEA_GROWTH},
 {AEA_MUTATE, AEA_ATTENTION, AEA_SHADOWS, AEA_NULL}
},
{"dead", "is icy cold to the touch", 
 {AEP_INVISIBLE, AEP_CLAIRVOYANCE, AEP_EXTINGUISH, AEP_SAP_LIFE},
 {AEP_HUNGER, AEP_EVIL, AEP_ALL_DOWN, AEP_SICK},
 {AEA_BLOOD, AEA_HURTALL, AEA_NULL, AEA_NULL},
 {AEA_PAIN, AEA_SHADOWS, AEA_DIM, AEA_VOMIT}
},
{"itchy", "makes your skin itch slightly when it is close", 
 {AEP_DEX_UP, AEP_SPEED_UP, AEP_PSYSHIELD, AEP_NULL},
 {AEP_RADIOACTIVE, AEP_MUTAGENIC, AEP_SICK, AEP_NULL},
 {AEA_ADRENALINE, AEA_BLOOD, AEA_HEAL, AEA_BUGS},
 {AEA_RADIATION, AEA_PAIN, AEA_PAIN, AEA_VOMIT}
},
{"glittering", "glitters faintly under direct light", 
 {AEP_INT_UP, AEP_EXTINGUISH, AEP_GLOW, AEP_NULL},
 {AEP_SMOKE, AEP_ATTENTION, AEP_NULL, AEP_NULL},
 {AEA_MAP, AEA_LIGHT, AEA_CONFUSED, AEA_ENTRANCE},
 {AEA_RADIATION, AEA_MUTATE, AEA_ATTENTION, AEA_FLASH}
},
{"electric", "very weakly shocks you when touched", 
 {AEP_RESIST_ELECTRICITY, AEP_DEX_UP, AEP_SPEED_UP, AEP_PSYSHIELD},
 {AEP_THIRST, AEP_SMOKE, AEP_STR_DOWN, AEP_BAD_WEATHER},
 {AEA_STORM, AEA_ADRENALINE, AEA_LIGHT, AEA_NULL},
 {AEA_PAIN, AEA_PARALYZE, AEA_FLASH, AEA_FLASH}
},
{"slimy", "feels slimy", 
 {AEP_SNAKES, AEP_STEALTH, AEP_EXTINGUISH, AEP_SAP_LIFE},
 {AEP_THIRST, AEP_DEX_DOWN, AEP_SPEED_DOWN, AEP_SICK},
 {AEA_BLOOD, AEA_ACIDBALL, AEA_GROWTH, AEA_ACIDBALL},
 {AEA_MUTATE, AEA_MUTATE, AEA_VOMIT, AEA_VOMIT}
},
{"engraved", "is covered with odd etchings", 
 {AEP_CLAIRVOYANCE, AEP_INVISIBLE, AEP_PSYSHIELD, AEP_SAP_LIFE},
 {AEP_EVIL, AEP_ATTENTION, AEP_NULL, AEP_NULL},
 {AEA_FATIGUE, AEA_TELEPORT, AEA_HEAL, AEA_FATIGUE},
 {AEA_ATTENTION, AEA_ATTENTION, AEA_TELEGLOW, AEA_DIM}
},
{"crackling", "occasionally makes a soft crackling sound", 
 {AEP_EXTINGUISH, AEP_RESIST_ELECTRICITY, AEP_NULL, AEP_NULL},
 {AEP_SMOKE, AEP_RADIOACTIVE, AEP_MOVEMENT_NOISE, AEP_NULL},
 {AEA_STORM, AEA_FIREBALL, AEA_PULSE, AEA_NULL},
 {AEA_PAIN, AEA_PARALYZE, AEA_NOISE, AEA_NOISE}
},
{"warm", "is warm to the touch", 
 {AEP_STR_UP, AEP_EXTINGUISH, AEP_GLOW, AEP_NULL},
 {AEP_SMOKE, AEP_RADIOACTIVE, AEP_NULL, AEP_NULL},
 {AEA_FIREBALL, AEA_FIREBALL, AEA_FIREBALL, AEA_LIGHT},
 {AEA_FIRESTORM, AEA_FIRESTORM, AEA_TELEGLOW, AEA_NULL}
},
{"rattling", "makes a rattling sound when moved", 
 {AEP_DEX_UP, AEP_SPEED_UP, AEP_SNAKES, AEP_CARRY_MORE},
 {AEP_ATTENTION, AEP_INT_DOWN, AEP_MOVEMENT_NOISE, AEP_MOVEMENT_NOISE},
 {AEA_BLOOD, AEA_PULSE, AEA_BUGS, AEA_NULL},
 {AEA_PAIN, AEA_ATTENTION, AEA_NOISE, AEA_NULL}
},
{"scaled", "has a surface reminiscent of reptile scales", 
 {AEP_SNAKES, AEP_SNAKES, AEP_SNAKES, AEP_STEALTH},
 {AEP_THIRST, AEP_MUTAGENIC, AEP_SPEED_DOWN, AEP_NULL},
 {AEA_ADRENALINE, AEA_BUGS, AEA_GROWTH, AEA_NULL},
 {AEA_MUTATE, AEA_SCREAM, AEA_DIM, AEA_NULL}
},
{"fractal",
"has a self-similar pattern which repeats until it is too small for you to see",
 {AEP_ALL_UP, AEP_ALL_UP, AEP_CLAIRVOYANCE, AEP_PSYSHIELD},
 {AEP_SCHIZO, AEP_ATTENTION, AEP_FORCE_TELEPORT, AEP_BAD_WEATHER},
 {AEA_STORM, AEA_FATIGUE, AEA_TELEPORT, AEA_NULL},
 {AEA_RADIATION, AEA_MUTATE, AEA_TELEGLOW, AEA_TELEGLOW}
}
};

enum artifact_weapon_type
{
 ARTWEAP_NULL,
 ARTWEAP_BULK,  // A bulky item that works okay for bashing
 ARTWEAP_CLUB,  // An item designed to bash
 ARTWEAP_SPEAR, // A stab-only weapon
 ARTWEAP_SWORD, // A long slasher
 ARTWEAP_KNIFE, // Short, slash and stab
 NUM_ARTWEAPS
};

struct artifact_tool_form_datum
{
 std::string name;
 char sym;
 nc_color color;
 material m1;
 material m2;
 int volume_min, volume_max;
 int weight_min, weight_max;
 artifact_weapon_type base_weapon;
 artifact_weapon_type extra_weapons[3];

/*
 artifact_tool_form_datum
   (std::string Name, char Sym, nc_color Color, material M1, material M2,
    int Volmin, int Volmax, int Wgtmin, int Wgtmax, artifact_weapon_type Base,
    artifact_weapon_type extra[3])
   : name (Name), sym (Sym), color (Color), m1 (M1), m2 (M2),
     volume_min (Volmin), volume_max (Volmax), weight_min (Wgtmin),
     weight_max (Wgtmax), base_weapon (Base), extra_weapons (extra) { };
*/
                           
};

enum artifact_tool_form
{
 ARTTOOLFORM_NULL,
 ARTTOOLFORM_HARP,
 ARTTOOLFORM_STAFF,
 ARTTOOLFORM_SWORD,
 ARTTOOLFORM_KNIFE,
 ARTTOOLFORM_CUBE,
 NUM_ARTTOOLFORMS
};

artifact_tool_form_datum artifact_tool_form_data[NUM_ARTTOOLFORMS] = {
{"", '*', c_white, MNULL, MNULL, 0, 0, 0, 0, ARTWEAP_BULK,
 {ARTWEAP_NULL, ARTWEAP_NULL, ARTWEAP_NULL}},

{"Harp", ';', c_yellow, WOOD, MNULL, 20, 30, 10, 18, ARTWEAP_BULK,
 {ARTWEAP_SPEAR, ARTWEAP_SWORD, ARTWEAP_KNIFE}},

{"Staff", '/', c_brown, WOOD, MNULL, 6, 12, 4, 10, ARTWEAP_CLUB,
 {ARTWEAP_BULK, ARTWEAP_SPEAR, ARTWEAP_KNIFE}},

{"Sword", '/', c_ltblue, STEEL, MNULL, 8, 14, 8, 28, ARTWEAP_SWORD,
 {ARTWEAP_BULK, ARTWEAP_NULL, ARTWEAP_NULL}},

{"Dagger", ';', c_ltblue, STEEL, MNULL, 1, 4, 1, 6, ARTWEAP_KNIFE,
 {ARTWEAP_NULL, ARTWEAP_NULL, ARTWEAP_NULL}},

{"Cube", '*', c_white, STEEL, MNULL, 1, 3, 1, 20, ARTWEAP_BULK,
 {ARTWEAP_SPEAR, ARTWEAP_NULL, ARTWEAP_NULL}}
};

struct artifact_weapon_datum
{
 std::string adjective;
 int volume, weight; // Only applicable if this is an *extra* weapon
 int bash_min, bash_max;
 int cut_min, cut_max;
 int to_hit_min, to_hit_max;
 unsigned flags : NUM_ITEM_FLAGS;
};

artifact_weapon_datum artifact_weapon_data[NUM_ARTWEAPS] = {
{"", 0, 0, 0, 0, 0, 0, 0, 0, 0},
// Adjective	Vol,wgt		Bash		Cut		To-Hit
{"Heavy",	 0, 12,		10, 20,		 0,  0,		-2,  0,
 0},
{"Knobbed",	 1,  2,		14, 30,		 0,  0,		-1,  1,
 0},
{"Spiked",	 1,  1,		 0,  0,		20, 40,		-1,  1,
 mfb(IF_SPEAR)},
{"Edged",	 2,  4,		 0,  0,		20, 50,		-1,  2,
 0},
{"Bladed",	 1,  2,		 0,  0,		12, 30,		-1,  1,
 mfb(IF_STAB)}
};

enum artifact_armor_mod
{
 ARMORMOD_NULL,
 ARMORMOD_LIGHT,
 ARMORMOD_BULKY,
 ARMORMOD_POCKETED,
 ARMORMOD_FURRED,
 ARMORMOD_PADDED,
 ARMORMOD_PLATED,
 NUM_ARMORMODS
};

struct artifact_armor_form_datum
{
 std::string name;
 nc_color color;
 material m1;
 material m2;
 int volume, weight;
 int encumb;
 int dmg_resist;
 int cut_resist;
 int env_resist;
 int warmth;
 int storage;
 int melee_bash, melee_cut, melee_hit;
 unsigned char covers;
 bool plural;
 artifact_armor_mod available_mods[5];

/*
// Constructor
 artifact_armor_form_datum
   (std::string Name, nc_color Color, material M1, material M2, int Volume,
    int Weight, int Encumb, int Dmg_res, int Cut_res, int Env_res,
    int Warmth, int Storage, int Bash, int Cut, int Hit, unsigned char Covers,
    bool Plural, artifact_armor_mod Mods[5])
   : name (Name), color (Color), m1 (M1), m2 (M2), volume (Volume),
     weight (Weight), encumb (Encumb), dmg_resist (Dmg_res),
     cut_resist (Cut_res), env_resist (Env_res), warmth (Warmth),
     storage (Storage), melee_bash (Bash), melee_cut (Cut), melee_hit (Hit),
     covers (Covers), plural (Plural), available_mods (Mods) { };

*/
};
 
enum artifact_armor_form
{
 ARTARMFORM_NULL,
 ARTARMFORM_ROBE,
 ARTARMFORM_COAT,
 ARTARMFORM_MASK,
 ARTARMFORM_HELM,
 ARTARMFORM_GLOVES,
 ARTARMFORM_BOOTS,
 ARTARMFORM_RING,
 NUM_ARTARMFORMS
};

artifact_armor_form_datum artifact_armor_form_data[NUM_ARTARMFORMS] = {
{"", c_white, MNULL, MNULL,        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0, false,
 {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}},
// Name    color  Materials      Vol Wgt Enc Dmg Cut Env Wrm Sto Bsh Cut Hit
{"Robe",   c_red, WOOL, MNULL,     6,  6,  1,  3,  1,  0,  2,  0, -8,  0, -3,
 mfb(bp_torso)|mfb(bp_legs), false,
 {ARMORMOD_LIGHT, ARMORMOD_BULKY, ARMORMOD_POCKETED, ARMORMOD_FURRED,
  ARMORMOD_PADDED}},

{"Coat",   c_brown,LEATHER,MNULL, 14, 14,  2,  4, 12,  1,  4,  4, -6,  0, -3,
 mfb(bp_torso), false,
 {ARMORMOD_LIGHT, ARMORMOD_POCKETED, ARMORMOD_FURRED, ARMORMOD_PADDED,
  ARMORMOD_PLATED}},

{"Mask",   c_white, WOOD, MNULL,   4,  1,  2,  2, 16,  1,  2,  0,  2,  0, -2,
 mfb(bp_eyes)|mfb(bp_mouth), false,
 {ARMORMOD_FURRED, ARMORMOD_FURRED, ARMORMOD_NULL, ARMORMOD_NULL,
  ARMORMOD_NULL}},

// Name    color  Materials      Vol Wgt Enc Dmg Cut Env Wrm Sto Bsh Cut Hit
{"Helm",   c_dkgray,SILVER, MNULL, 6,  6,  2,  4, 18,  0,  1,  0,  8,  0, -2,
 mfb(bp_head), false,
 {ARMORMOD_BULKY, ARMORMOD_FURRED, ARMORMOD_PADDED, ARMORMOD_PLATED,
  ARMORMOD_NULL}},

{"Gloves", c_ltblue,LEATHER,MNULL, 2,  1,  1,  6,  6,  1,  2,  0, -4,  0, -2,
 mfb(bp_hands), true,
 {ARMORMOD_BULKY, ARMORMOD_FURRED, ARMORMOD_PADDED, ARMORMOD_PLATED, 
  ARMORMOD_NULL}},

// Name    color  Materials      Vol Wgt Enc Dmg Cut Env Wrm Sto Bsh Cut Hit
{"Boots", c_blue, LEATHER, MNULL,  6,  2,  1,  6,  6,  1,  3,  0,  4,  0, -1,
 mfb(bp_feet), true,
 {ARMORMOD_LIGHT, ARMORMOD_BULKY, ARMORMOD_PADDED, ARMORMOD_PLATED,
  ARMORMOD_NULL}},

{"Ring", c_ltgreen, SILVER, MNULL,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0, true,
 {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}}

};

/* Armor mods alter the normal values of armor.
 * If the basic armor type has MNULL as its second material, and the mod has a
 * material attached, the second material will be changed.
 */
artifact_armor_form_datum artifact_armor_mod_data[NUM_ARMORMODS] = {

{"", c_white, MNULL, MNULL, 0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, false,
 {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}},
// Description; "It is ..." or "They are ..."
{"very thin and light.", c_white, MNULL, MNULL,
// Vol Wgt Enc Dmg Cut Env Wrm Sto
    -4, -8, -2, -1, -1, -1, -1,  0, 0, 0, 0, 0,  false,
 {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}},

{"extremely bulky.", c_white, MNULL, MNULL,
     8, 10,  2,  1,  1,  0,  1,  0, 0, 0, 0, 0,  false,
 {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}},

{"covered in pockets.", c_white, MNULL, MNULL,
     1,  1,  1,  0,  0,  0,  0, 16, 0, 0, 0, 0,  false,
 {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}},

{"disgustingly furry.", c_white, WOOL, MNULL,
// Vol Wgt Enc Dmg Cut Env Wrm Sto
     4,  2,  1,  4,  0,  1,  3,  0, 0, 0, 0, 0,  false,
 {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}},

{"leather-padded.", c_white, LEATHER, MNULL,
     4,  4,  1, 10,  4,  0,  1, -3, 0, 0, 0, 0,  false,
 {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}},

{"plated in iron.", c_white, IRON, MNULL,
     4, 12,  3,  8, 14,  0,  1, -4, 0, 0, 0, 0, false,
 {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}},

};


#define NUM_ART_ADJS 20
std::string artifact_adj[NUM_ART_ADJS] = {
"Forbidden", "Unknown", "Forgotten", "Hideous", "Eldritch",
"Gelatinous", "Ancient", "Cursed", "Bloody", "Undying",
"Shadowy", "Silent", "Cyclopean", "Fungal", "Unspeakable",
"Grotesque", "Frigid", "Shattered", "Sleeping", "Repellent"
};

#define NUM_ART_NOUNS 20
// Prepending + makes it proper, e.g. "The Forbidden Abyss"
std::string artifact_noun[NUM_ART_NOUNS] = {
"Technique", "Dreams", "Beasts", "Evil", "Miasma",
"+Abyss", "+City", "Shadows", "Shade", "Illusion",
"Justice", "+Necropolis", "Ichor", "+Monolith", "Aeons",
"Graves", "Horrors", "Suffering", "Death", "Horror"
};

#endif
