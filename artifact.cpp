#include <sstream>
#include <vector>
#include "game.h"
#include "artifact.h"
#include "artifactdata.h"

std::vector<art_effect_passive> fill_good_passive();
std::vector<art_effect_passive> fill_bad_passive();
std::vector<art_effect_active>  fill_good_active();
std::vector<art_effect_active>  fill_bad_active();

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
50,	// AEP_SUPER_CLAIRVOYANCE
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

std::string artifact_adj[NUM_ART_ADJS] = {
"Forbidden", "Unknown", "Forgotten", "Hideous", "Eldritch",
"Gelatinous", "Ancient", "Cursed", "Bloody", "Undying",
"Shadowy", "Silent", "Cyclopean", "Fungal", "Unspeakable",
"Grotesque", "Frigid", "Shattered", "Sleeping", "Repellent"
};

// Prepending + makes it proper, e.g. "The Forbidden Abyss"
std::string artifact_noun[NUM_ART_NOUNS] = {
"Technique", "Dreams", "Beasts", "Evil", "Miasma",
"+Abyss", "+City", "Shadows", "Shade", "Illusion",
"Justice", "+Necropolis", "Ichor", "+Monolith", "Aeons",
"Graves", "Horrors", "Suffering", "Death", "Horror"
};

std::string artifact_name(std::string type);

itype* game::new_artifact()
{
 if (one_in(2)) { // Generate a "tool" artifact

  it_artifact_tool *art = new it_artifact_tool();
  int form = rng(ARTTOOLFORM_NULL + 1, NUM_ARTTOOLFORMS - 1);

  artifact_tool_form_datum *info = &(artifact_tool_form_data[form]);
  art->name = artifact_name(info->name);
  art->color = info->color;
  art->sym = info->sym;
  art->m1 = info->m1;
  art->m2 = info->m2;
  art->volume = rng(info->volume_min, info->volume_max);
  art->weight = rng(info->weight_min, info->weight_max);
// Set up the basic weapon type
  artifact_weapon_datum *weapon = &(artifact_weapon_data[info->base_weapon]);
  art->melee_dam = rng(weapon->bash_min, weapon->bash_max);
  art->melee_cut = rng(weapon->cut_min, weapon->cut_max);
  art->m_to_hit = rng(weapon->to_hit_min, weapon->to_hit_max);
  art->item_flags = weapon->flags;
// Add an extra weapon perhaps?
  if (one_in(2)) {
   int select = rng(0, 2);
   if (info->extra_weapons[select] != ARTWEAP_NULL) {
    weapon = &(artifact_weapon_data[ info->extra_weapons[select] ]);
    art->volume += weapon->volume;
    art->weight += weapon->weight;
    art->melee_dam += rng(weapon->bash_min, weapon->bash_max);
    art->melee_cut += rng(weapon->bash_min, weapon->bash_max);
    art->m_to_hit += rng(weapon->to_hit_min, weapon->to_hit_max);
    art->item_flags |= weapon->flags;
    std::stringstream newname;
    newname << weapon->adjective << " " << info->name;
    art->name = artifact_name(newname.str());
   }
  }

  std::stringstream description;
  description << "This is the " << art->name << ".\n\
It is the only one of its kind.\n\
It may have unknown powers; use 'a' to activate them.";
  art->description = description.str();

// Finally, pick some powers
  art_effect_passive passive_tmp = AEP_NULL;
  art_effect_active active_tmp = AEA_NULL;
  int num_good = 0, num_bad = 0, value = 0;
  std::vector<art_effect_passive> good_effects = fill_good_passive();
  std::vector<art_effect_passive> bad_effects = fill_bad_passive();

// Wielded effects first
  while (!good_effects.empty() && !bad_effects.empty() &&
         num_good < 3 && num_bad < 3 &&
         (num_good < 1 || num_bad < 1 || one_in(num_good + 1) ||
          one_in(num_bad + 1) || value > 1)) {
   if (value < 1 && one_in(2)) { // Good
    int index = rng(0, good_effects.size() - 1);
    passive_tmp = good_effects[index];
    good_effects.erase(good_effects.begin() + index);
    num_good++;
   } else if (!bad_effects.empty()) { // Bad effect
    int index = rng(0, bad_effects.size() - 1);
    passive_tmp = bad_effects[index];
    bad_effects.erase(bad_effects.begin() + index);
    num_bad++;
   }
   value += passive_effect_cost[passive_tmp];
   art->effects_wielded.push_back(passive_tmp);
  }
// Next, carried effects; more likely to be just bad
  num_good = 0;
  num_bad = 0;
  value = 0;
  good_effects = fill_good_passive();
  bad_effects = fill_bad_passive();
  while (one_in(2) && !good_effects.empty() && !bad_effects.empty() &&
         num_good < 3 && num_bad < 3 &&
         ((num_good > 2 && one_in(num_good + 1)) || num_bad < 1 ||
          one_in(num_bad + 1) || value > 1)) {
   if (value < 1 && one_in(3)) { // Good
    int index = rng(0, good_effects.size() - 1);
    passive_tmp = good_effects[index];
    good_effects.erase(good_effects.begin() + index);
    num_good++;
   } else { // Bad effect
    int index = rng(0, bad_effects.size() - 1);
    passive_tmp = bad_effects[index];
    bad_effects.erase(bad_effects.begin() + index);
    num_bad++;
   }
   value += passive_effect_cost[passive_tmp];
   art->effects_carried.push_back(passive_tmp);
  }
// Finally, activated effects; not necessarily good or bad
  num_good = 0;
  num_bad = 0;
  value = 0;
  art->def_charges = 0;
  art->max_charges = 0;
  std::vector<art_effect_active> good_a_effects = fill_good_active();
  std::vector<art_effect_active> bad_a_effects = fill_bad_active();
  while (!good_a_effects.empty() && !bad_a_effects.empty() &&
         num_good < 3 && num_bad < 3 &&
         (value > 3 || (num_bad > 0 && num_good == 0) ||
          !one_in(3 - num_good) || !one_in(3 - num_bad))) {
   if (!one_in(3) && value <= 1) { // Good effect
    int index = rng(0, good_a_effects.size() - 1);
    active_tmp = good_a_effects[index];
    good_a_effects.erase(good_a_effects.begin() + index);
    num_good++;
    value += active_effect_cost[active_tmp];
   } else { // Bad effect
    int index = rng(0, bad_a_effects.size() - 1);
    active_tmp = bad_a_effects[index];
    bad_a_effects.erase(bad_a_effects.begin() + index);
    num_bad++;
    value += active_effect_cost[active_tmp];
   }
   art->effects_activated.push_back(active_tmp);
   art->max_charges += rng(1, 3);
  }
  art->def_charges = art->max_charges;
// If we have charges, pick a recharge mechanism
  if (art->max_charges > 0)
   art->charge_type = art_charge( rng(ARTC_NULL + 1, NUM_ARTCS - 1) );
  if (one_in(8) && num_bad + num_good >= 4)
   art->charge_type = ARTC_NULL; // 1 in 8 chance that it can't recharge!

  art->id = itypes.size();
  itypes[art->name]=art;
  return art;

 } else { // Generate an armor artifact

  it_artifact_armor *art = new it_artifact_armor();
  int form = rng(ARTARMFORM_NULL + 1, NUM_ARTARMFORMS - 1);
  artifact_armor_form_datum *info = &(artifact_armor_form_data[form]);

  art->name = artifact_name(info->name);
  art->sym = '['; // Armor is always [
  art->color = info->color;
  art->m1 = info->m1;
  art->m2 = info->m2;
  art->volume = info->volume;
  art->weight = info->weight;
  art->melee_dam = info->melee_bash;
  art->melee_cut = info->melee_cut;
  art->m_to_hit = info->melee_hit;
  art->item_flags = 0;
  art->covers = info->covers;
  art->encumber = info->encumb;
  art->dmg_resist = info->dmg_resist;
  art->cut_resist = info->cut_resist;
  art->env_resist = info->env_resist;
  art->warmth = info->warmth;
  art->storage = info->storage;
  std::stringstream description;
  description << "This is the " << art->name << ".\n" <<
                 (info->plural ? "They are the only ones of their kind." :
                                 "It is the only one of its kind.");

// Modify the armor further
  if (!one_in(4)) {
   int index = rng(0, 4);
   if (info->available_mods[index] != ARMORMOD_NULL) {
    artifact_armor_mod mod = info->available_mods[index];
    artifact_armor_form_datum *modinfo = &(artifact_armor_mod_data[mod]);
    if (modinfo->volume >= 0 || art->volume > abs(modinfo->volume))
     art->volume += modinfo->volume;
    else
     art->volume = 1;

    if (modinfo->weight >= 0 || art->weight > abs(modinfo->weight))
     art->weight += modinfo->weight;
    else
     art->weight = 1;

    art->encumber += modinfo->encumb;

    if (modinfo->dmg_resist > 0 || art->dmg_resist > abs(modinfo->dmg_resist))
     art->dmg_resist += modinfo->dmg_resist;
    else
     art->dmg_resist = 0;

    if (modinfo->cut_resist > 0 || art->cut_resist > abs(modinfo->cut_resist))
     art->cut_resist += modinfo->cut_resist;
    else
     art->cut_resist = 0;

    if (modinfo->env_resist > 0 || art->env_resist > abs(modinfo->env_resist))
     art->env_resist += modinfo->env_resist;
    else
     art->env_resist = 0;
    art->warmth += modinfo->warmth;

    if (modinfo->storage > 0 || art->storage > abs(modinfo->storage))
     art->storage += modinfo->storage;
    else
     art->storage = 0;

    description << "\n" << (info->plural ? "They are " : "It is ") <<
                   modinfo->name;
   }
  }

  art->description = description.str();

// Finally, pick some effects
  int num_good = 0, num_bad = 0, value = 0;
  art_effect_passive passive_tmp = AEP_NULL;
  std::vector<art_effect_passive> good_effects = fill_good_passive();
  std::vector<art_effect_passive> bad_effects = fill_bad_passive();

  while (!good_effects.empty() && !bad_effects.empty() &&
         num_good < 3 && num_bad < 3 &&
         (num_good < 1 || one_in(num_good * 2) || value > 1 ||
          (num_bad < 3 && !one_in(3 - num_bad)))) {
   if (value < 1 && one_in(2)) { // Good effect
    int index = rng(0, good_effects.size() - 1);
    passive_tmp = good_effects[index];
    good_effects.erase(good_effects.begin() + index);
    num_good++;
   } else { // Bad effect
    int index = rng(0, bad_effects.size() - 1);
    passive_tmp = bad_effects[index];
    bad_effects.erase(bad_effects.begin() + index);
    num_bad++;
   }
   value += passive_effect_cost[passive_tmp];
   art->effects_worn.push_back(passive_tmp);
  }

  art->id = itypes.size();
  itypes[art->name]=art;
  return art;
 }
}

itype* game::new_natural_artifact(artifact_natural_property prop)
{
// Natural artifacts are always tools.
 it_artifact_tool *art = new it_artifact_tool();
// Pick a form
 artifact_natural_shape shape =
               artifact_natural_shape(rng(ARTSHAPE_NULL + 1, ARTSHAPE_MAX - 1));
 artifact_shape_datum *shape_data = &(artifact_shape_data[shape]);
// Pick a property
 artifact_natural_property property = (prop > ARTPROP_NULL ? prop :
             artifact_natural_property(rng(ARTPROP_NULL + 1, ARTPROP_MAX - 1)));
 artifact_property_datum *property_data = &(artifact_property_data[property]);

 art->sym = ':';
 art->color = c_yellow;
 art->m1 = STONE;
 art->m2 = MNULL;
 art->volume = rng(shape_data->volume_min, shape_data->volume_max);
 art->weight = rng(shape_data->weight_min, shape_data->weight_max);
 art->melee_dam = 0;
 art->melee_cut = 0;
 art->m_to_hit = 0;
 art->item_flags = 0;

 art->name = property_data->name + " " + shape_data->name;
 std::stringstream desc;
 desc << "This " << shape_data->desc << " " << property_data->desc << ".";
 art->description = desc.str();
// Add line breaks to the description as necessary
 size_t pos = 76;
 while (art->description.length() - pos >= 76) {
  pos = art->description.find_last_of(' ', pos);
  if (pos == std::string::npos)
   pos = art->description.length();
  else {
   art->description[pos] = '\n';
   pos += 76;
  }
 }

// Three possibilities: good passive + bad passive, good active + bad active,
// and bad passive + good active
 bool good_passive = false, bad_passive = false,
      good_active  = false, bad_active  = false;
 switch (rng(1, 3)) {
  case 1:
   good_passive = true;
   bad_passive  = true;
   break;
  case 2:
   good_active = true;
   bad_active  = true;
   break;
  case 3:
   bad_passive = true;
   good_active = true;
   break;
 }

 int value_to_reach = 0; // This is slowly incremented, allowing for better arts
 int value = 0;
 art_effect_passive aep_good = AEP_NULL, aep_bad = AEP_NULL;
 art_effect_active  aea_good = AEA_NULL, aea_bad = AEA_NULL;

 do {
  if (good_passive) {
   aep_good = property_data->passive_good[ rng(0, 3) ];
   if (aep_good == AEP_NULL || one_in(4))
    aep_good = art_effect_passive(rng(AEP_NULL + 1, AEP_SPLIT - 1));
  }
  if (bad_passive) {
   aep_bad = property_data->passive_bad[ rng(0, 3) ];
   if (aep_bad == AEP_NULL || one_in(4))
    aep_bad = art_effect_passive(rng(AEP_SPLIT + 1, NUM_AEAS - 1));
  }
  if (good_active) {
   aea_good = property_data->active_good[ rng(0, 3) ];
   if (aea_good == AEA_NULL || one_in(4))
    aea_good = art_effect_active(rng(AEA_NULL + 1, AEA_SPLIT - 1));
  }
  if (bad_active) {
   aea_bad = property_data->active_bad[ rng(0, 3) ];
   if (aea_bad == AEA_NULL || one_in(4))
    aea_bad = art_effect_active(rng(AEA_SPLIT + 1, NUM_AEAS - 1));
  }

  value = passive_effect_cost[aep_good] + passive_effect_cost[aep_bad] +
          active_effect_cost[aea_good] +  active_effect_cost[aea_bad];
  value_to_reach++; // Yes, it is intentional that this is 1 the first check
 } while (value > value_to_reach);

 if (aep_good != AEP_NULL)
  art->effects_carried.push_back(aep_good);
 if (aep_bad != AEP_NULL)
  art->effects_carried.push_back(aep_bad);
 if (aea_good != AEA_NULL)
  art->effects_activated.push_back(aea_good);
 if (aea_bad != AEA_NULL)
  art->effects_activated.push_back(aea_bad);

// Natural artifacts ALWAYS can recharge
// (When "implanting" them in a mundane item, this ability may be lost
 if (!art->effects_activated.empty()) {
  art->max_charges = rng(1, 4);
  art->def_charges = art->max_charges;
  art->charge_type = art_charge( rng(ARTC_NULL + 1, NUM_ARTCS - 1) );
 }

 art->id = itypes.size();
 itypes[art->name]=art;
 return art;
}

std::vector<art_effect_passive> fill_good_passive()
{
 std::vector<art_effect_passive> ret;
 for (int i = AEP_NULL + 1; i < AEP_SPLIT; i++)
  ret.push_back( art_effect_passive(i) );
 return ret;
}

std::vector<art_effect_passive> fill_bad_passive()
{
 std::vector<art_effect_passive> ret;
 for (int i = AEP_SPLIT + 1; i < NUM_AEPS; i++)
  ret.push_back( art_effect_passive(i) );
 return ret;
}

std::vector<art_effect_active> fill_good_active()
{
 std::vector<art_effect_active> ret;
 for (int i = AEA_NULL + 1; i < AEA_SPLIT; i++)
  ret.push_back( art_effect_active(i) );
 return ret;
}

std::vector<art_effect_active> fill_bad_active()
{
 std::vector<art_effect_active> ret;
 for (int i = AEA_SPLIT + 1; i < NUM_AEAS; i++)
  ret.push_back( art_effect_active(i) );
 return ret;
}

std::string artifact_name(std::string type)
{
 std::stringstream ret;
 ret << type << " of ";
 std::string noun = artifact_noun[rng(0, NUM_ART_NOUNS - 1)];
 if (noun[0] == '+') {
  ret << "the ";
  noun = noun.substr(1); // Chop off '+'
 }
 ret << artifact_adj[rng(0, NUM_ART_ADJS - 1)] << " " << noun;
 return ret.str();
}


void game::process_artifact(item *it, player *p, bool wielded)
{
 std::vector<art_effect_passive> effects;
 if (it->is_armor()) {
  it_artifact_armor* armor = dynamic_cast<it_artifact_armor*>(it->type);
  effects = armor->effects_worn;
 } else if (it->is_tool()) {
  it_artifact_tool* tool = dynamic_cast<it_artifact_tool*>(it->type);
  effects = tool->effects_carried;
  if (wielded) {
   for (int i = 0; i < tool->effects_wielded.size(); i++)
    effects.push_back(tool->effects_wielded[i]);
  }
// Recharge it if necessary
  if (it->charges < tool->max_charges) {
   switch (tool->charge_type) {
    case ARTC_TIME:
     if (turn.seconds() == 0 && turn.minutes() == 0) // Once per hour
      it->charges++;
     break;
    case ARTC_SOLAR:
     if (turn.seconds() == 0 && turn.minutes() % 10 == 0 &&
         is_in_sunlight(p->posx, p->posy))
      it->charges++;
     break;
    case ARTC_PAIN:
     if (turn.seconds() == 0) {
      add_msg("You suddenly feel sharp pain for no reason.");
      p->pain += 3 * rng(1, 3);
      it->charges++;
     }
     break;
    case ARTC_HP:
     if (turn.seconds() == 0) {
      add_msg("You feel your body decaying.");
      p->hurtall(1);
      it->charges++;
     }
     break;
   }
  }
 }

 for (int i = 0; i < effects.size(); i++) {
  switch (effects[i]) {
  case AEP_STR_UP:
   p->str_cur += 4;
   break;
  case AEP_DEX_UP:
   p->dex_cur += 4;
   break;
  case AEP_PER_UP:
   p->per_cur += 4;
   break;
  case AEP_INT_UP:
   p->int_cur += 4;
   break;
  case AEP_ALL_UP:
   p->str_cur += 2;
   p->dex_cur += 2;
   p->per_cur += 2;
   p->int_cur += 2;
   break;
  case AEP_SPEED_UP: // Handled in player::current_speed()
   break;

  case AEP_IODINE:
   if (p->radiation > 0)
    p->radiation--;
   break;

  case AEP_SMOKE:
   if (one_in(10)) {
    int x = p->posx + rng(-1, 1), y = p->posy + rng(-1, 1);
    if (m.add_field(this, x, y, fd_smoke, rng(1, 3)))
     add_msg("The %s emits some smoke.", it->tname().c_str());
   }
   break;

  case AEP_SNAKES:
   break; // Handled in player::hit()

  case AEP_EXTINGUISH:
   for (int x = p->posx - 1; x <= p->posx + 1; x++) {
    for (int y = p->posy - 1; y <= p->posy + 1; y++) {
     if (m.field_at(x, y).type == fd_fire) {
      if (m.field_at(x, y).density == 0)
       m.remove_field(x, y);
      else
       m.field_at(x, y).density--;
     }
    }
   }
   break;

  case AEP_HUNGER:
   if (one_in(100))
    p->hunger++;
   break;

  case AEP_THIRST:
   if (one_in(120))
    p->thirst++;
   break;

  case AEP_EVIL:
   if (one_in(150)) { // Once every 15 minutes, on average
    p->add_disease(DI_EVIL, 300, this);
    if (!wielded && !it->is_armor())
     add_msg("You have an urge to %s the %s.",
             (it->is_armor() ? "wear" : "wield"), it->tname().c_str());
   }
   break;

  case AEP_SCHIZO:
   break; // Handled in player::suffer()

  case AEP_RADIOACTIVE:
   if (one_in(4))
    p->radiation++;
   break;

  case AEP_STR_DOWN:
   p->str_cur -= 3;
   break;

  case AEP_DEX_DOWN:
   p->dex_cur -= 3;
   break;

  case AEP_PER_DOWN:
   p->per_cur -= 3;
   break;

  case AEP_INT_DOWN:
   p->int_cur -= 3;
   break;

  case AEP_ALL_DOWN:
   p->str_cur -= 2;
   p->dex_cur -= 2;
   p->per_cur -= 2;
   p->int_cur -= 2;
   break;

  case AEP_SPEED_DOWN:
   break; // Handled in player::current_speed()
  }
 }
}

void game::add_artifact_messages(std::vector<art_effect_passive> effects)
{
 int net_str = 0, net_dex = 0, net_per = 0, net_int = 0, net_speed = 0;
 for (int i = 0; i < effects.size(); i++) {
  switch (effects[i]) {
   case AEP_STR_UP:   net_str += 4; break;
   case AEP_DEX_UP:   net_dex += 4; break;
   case AEP_PER_UP:   net_per += 4; break;
   case AEP_INT_UP:   net_int += 4; break;
   case AEP_ALL_UP:   net_str += 2;
                      net_dex += 2;
                      net_per += 2;
                      net_int += 2; break;
   case AEP_STR_DOWN: net_str -= 3; break;
   case AEP_DEX_DOWN: net_dex -= 3; break;
   case AEP_PER_DOWN: net_per -= 3; break;
   case AEP_INT_DOWN: net_int -= 3; break;
   case AEP_ALL_DOWN: net_str -= 2;
                      net_dex -= 2;
                      net_per -= 2;
                      net_int -= 2; break;

   case AEP_SPEED_UP:   net_speed += 20; break;
   case AEP_SPEED_DOWN: net_speed -= 20; break;

   case AEP_IODINE:
    break; // No message

   case AEP_SNAKES:
    add_msg("Your skin feels slithery.");
    break;

   case AEP_INVISIBLE:
    add_msg("You fade into invisibility!");
    break;

   case AEP_CLAIRVOYANCE:
    add_msg("You can see through walls!");
    break;

   case AEP_SUPER_CLAIRVOYANCE:
    add_msg("You can see through everything!");
    break;

   case AEP_STEALTH:
    add_msg("Your steps stop making noise.");
    break;

   case AEP_GLOW:
    add_msg("A glow of light forms around you.");
    break;

   case AEP_PSYSHIELD:
    add_msg("Your mental state feels protected.");
    break;

   case AEP_RESIST_ELECTRICITY:
    add_msg("You feel insulated.");
    break;

   case AEP_CARRY_MORE:
    add_msg("Your back feels strengthened.");
    break;

   case AEP_HUNGER:
    add_msg("You feel hungry.");
    break;

   case AEP_THIRST:
    add_msg("You feel thirsty.");
    break;

   case AEP_EVIL:
    add_msg("You feel an evil presence...");
    break;

   case AEP_SCHIZO:
    add_msg("You feel a tickle of insanity.");
    break;

   case AEP_RADIOACTIVE:
    add_msg("Your skin prickles with radiation.");
    break;

   case AEP_MUTAGENIC:
    add_msg("You feel your genetic makeup degrading.");
    break;

   case AEP_ATTENTION:
    add_msg("You feel an otherworldly attention upon you...");
    break;

   case AEP_FORCE_TELEPORT:
    add_msg("You feel a force pulling you inwards.");
    break;

   case AEP_MOVEMENT_NOISE:
    add_msg("You hear a rattling noise coming from inside yourself.");
    break;

   case AEP_BAD_WEATHER:
    add_msg("You feel storms coming.");
    break;
  }
 }

 std::stringstream stat_info;
 if (net_str != 0)
  stat_info << "Str " << (net_str > 0 ? "+" : "") << net_str << "! ";
 if (net_dex != 0)
  stat_info << "Dex " << (net_dex > 0 ? "+" : "") << net_dex << "! ";
 if (net_int != 0)
  stat_info << "Int " << (net_int > 0 ? "+" : "") << net_int << "! ";
 if (net_per != 0)
  stat_info << "Per " << (net_per > 0 ? "+" : "") << net_per << "! ";

 if (stat_info.str().length() > 0)
  add_msg(stat_info.str().c_str());

 if (net_speed != 0)
  add_msg("Speed %s%d", (net_speed > 0 ? "+" : ""), net_speed);
}
