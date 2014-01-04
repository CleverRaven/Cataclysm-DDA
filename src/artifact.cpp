#include "artifact.h"

#include "itype.h"
#include "output.h" // string_format
#include "json.h"

#include <string>
#include <vector>
#include <sstream>
#include <fstream>

std::vector<art_effect_passive> fill_good_passive();
std::vector<art_effect_passive> fill_bad_passive();
std::vector<art_effect_active>  fill_good_active();
std::vector<art_effect_active>  fill_bad_active();

int passive_effect_cost[NUM_AEPS] = {
0, // AEP_NULL

3, // AEP_STR_UP
3, // AEP_DEX_UP
3, // AEP_PER_UP
3, // AEP_INT_UP
5, // AEP_ALL_UP
4, // AEP_SPEED_UP
2, // AEP_IODINE
4, // AEP_SNAKES
7, // AEP_INVISIBLE
5, // AEP_CLAIRVOYANCE
50,// AEP_SUPER_CLAIRVOYANCE
2, // AEP_STEALTH
2, // AEP_EXTINGUISH
1, // AEP_GLOW
1, // AEP_PSYSHIELD
3, // AEP_RESIST_ELECTRICITY
3, // AEP_CARRY_MORE
5, // AEP_SAP_LIFE

0, // AEP_SPLIT

-2, // AEP_HUNGER
-2, // AEP_THIRST
-1, // AEP_SMOKE
-5, // AEP_EVIL
-3, // AEP_SCHIZO
-5, // AEP_RADIOACTIVE
-3, // AEP_MUTAGENIC
-5, // AEP_ATTENTION
-2, // AEP_STR_DOWN
-2, // AEP_DEX_DOWN
-2, // AEP_PER_DOWN
-2, // AEP_INT_DOWN
-5, // AEP_ALL_DOWN
-4, // AEP_SPEED_DOWN
-5, // AEP_FORCE_TELEPORT
-3, // AEP_MOVEMENT_NOISE
-2, // AEP_BAD_WEATHER
-1  // AEP_SICK
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

std::string mk_artifact_id() {
    char buff[32];
    sprintf(buff,"artifact_%zu", artifact_itype_ids.size());
    return buff;
};

//see below, move them so gettext and be applied properly
artifact_shape_datum artifact_shape_data[ARTSHAPE_MAX];
artifact_property_datum artifact_property_data[ARTPROP_MAX];
artifact_tool_form_datum artifact_tool_form_data[NUM_ARTTOOLFORMS];
artifact_weapon_datum artifact_weapon_data[NUM_ARTWEAPS];
artifact_armor_form_datum artifact_armor_form_data[NUM_ARTARMFORMS];
artifact_armor_form_datum artifact_armor_mod_data[NUM_ARMORMODS];
std::string artifact_adj[NUM_ART_ADJS];
std::string artifact_noun[NUM_ART_NOUNS];
std::string artifact_name(std::string type);

void init_artifacts()
{
    artifact_shape_datum tmp_artifact_shape_data[ARTSHAPE_MAX] = {
    {"BUG", "BUG", 0, 0, 0, 0},
    {_("sphere"), _("smooth sphere"), 2, 4, 1, 1150},
    {_("rod"), _("tapered rod"), 1, 7, 1, 800},
    {_("teardrop"), _("teardrop-shaped stone"), 2, 6, 1, 950},
    {_("lamp"), _("hollow, transparent cube"), 4, 9, 1, 350},
    {_("snake"), _("winding, flexible rod"), 0, 8, 1, 950},
    {_("disc"), _("smooth disc"), 4, 6, 200, 400},
    {_("beads"), _("string of beads"), 3, 7, 1, 700},
    {_("napkin"), _("very thin sheet"), 0, 3, 1, 350},
    {_("urchin"), _("spiked sphere"), 3, 5, 200, 700},
    {_("jelly"), _("malleable blob"), 2, 8, 200, 450},
    {_("spiral"), _("spiraling rod"), 5, 6, 200, 350},
    {_("pin"), _("pointed rod"), 1, 5, 100, 1050},
    {_("tube"), _("hollow tube"), 2, 5, 350, 700},
    {_("pyramid"), _("regular tetrahedron"), 3, 7, 200, 450},
    {_("crystal"), _("translucent crystal"), 1, 6, 200, 800},
    {_("knot"), _("twisted, knotted cord"), 2, 6, 100, 800},
    {_("crescent"), _("crescent-shaped stone"), 2, 6, 200, 700}
    };
    for(int i=0;i<ARTSHAPE_MAX;i++) {artifact_shape_data[i]=tmp_artifact_shape_data[i];}

    artifact_property_datum tmp_artifact_property_data[ARTPROP_MAX] = {
    {"BUG", "BUG",
     {AEP_NULL, AEP_NULL, AEP_NULL, AEP_NULL},
     {AEP_NULL, AEP_NULL, AEP_NULL, AEP_NULL},
     {AEA_NULL, AEA_NULL, AEA_NULL, AEA_NULL},
     {AEA_NULL, AEA_NULL, AEA_NULL, AEA_NULL}
    },
    {_("wriggling"), _("is constantly wriggling"),
     {AEP_SPEED_UP, AEP_SNAKES, AEP_NULL, AEP_NULL},
     {AEP_DEX_DOWN, AEP_FORCE_TELEPORT, AEP_SICK, AEP_NULL},
     {AEA_TELEPORT, AEA_ADRENALINE, AEA_NULL, AEA_NULL},
     {AEA_MUTATE, AEA_ATTENTION, AEA_VOMIT, AEA_NULL}
    },
    {_("glowing"), _("glows faintly"),
     {AEP_INT_UP, AEP_GLOW, AEP_CLAIRVOYANCE, AEP_NULL},
     {AEP_RADIOACTIVE, AEP_MUTAGENIC, AEP_ATTENTION, AEP_NULL},
     {AEA_LIGHT, AEA_LIGHT, AEA_LIGHT, AEA_NULL},
     {AEA_ATTENTION, AEA_TELEGLOW, AEA_FLASH, AEA_SHADOWS}
    },
    {_("humming"), _("hums very quietly"),
     {AEP_ALL_UP, AEP_PSYSHIELD, AEP_NULL, AEP_NULL},
     {AEP_SCHIZO, AEP_PER_DOWN, AEP_INT_DOWN, AEP_NULL},
     {AEA_PULSE, AEA_ENTRANCE, AEA_NULL, AEA_NULL},
     {AEA_NOISE, AEA_NOISE, AEA_SCREAM, AEA_NULL}
    },
    {_("moving"), _("shifts from side to side slowly"),
     {AEP_STR_UP, AEP_DEX_UP, AEP_SPEED_UP, AEP_NULL},
     {AEP_HUNGER, AEP_PER_DOWN, AEP_FORCE_TELEPORT, AEP_NULL},
     {AEA_TELEPORT, AEA_TELEPORT, AEA_MAP, AEA_NULL},
     {AEA_PARALYZE, AEA_VOMIT, AEA_VOMIT, AEA_NULL}
    },
    {_("whispering"), _("makes very faint whispering sounds"),
     {AEP_CLAIRVOYANCE, AEP_EXTINGUISH, AEP_STEALTH, AEP_NULL},
     {AEP_EVIL, AEP_SCHIZO, AEP_ATTENTION, AEP_NULL},
     {AEA_FATIGUE, AEA_ENTRANCE, AEA_ENTRANCE, AEA_NULL},
     {AEA_ATTENTION, AEA_SCREAM, AEA_SCREAM, AEA_SHADOWS}
    },
    {_("breathing"),
     _("shrinks and grows very slightly with a regular pulse, as if breathing"),
     {AEP_SAP_LIFE, AEP_ALL_UP, AEP_SPEED_UP, AEP_CARRY_MORE},
     {AEP_HUNGER, AEP_THIRST, AEP_SICK, AEP_BAD_WEATHER},
     {AEA_ADRENALINE, AEA_HEAL, AEA_ENTRANCE, AEA_GROWTH},
     {AEA_MUTATE, AEA_ATTENTION, AEA_SHADOWS, AEA_NULL}
    },
    {_("dead"), _("is icy cold to the touch"),
     {AEP_INVISIBLE, AEP_CLAIRVOYANCE, AEP_EXTINGUISH, AEP_SAP_LIFE},
     {AEP_HUNGER, AEP_EVIL, AEP_ALL_DOWN, AEP_SICK},
     {AEA_BLOOD, AEA_HURTALL, AEA_NULL, AEA_NULL},
     {AEA_PAIN, AEA_SHADOWS, AEA_DIM, AEA_VOMIT}
    },
    {_("itchy"), _("makes your skin itch slightly when it is close"),
     {AEP_DEX_UP, AEP_SPEED_UP, AEP_PSYSHIELD, AEP_NULL},
     {AEP_RADIOACTIVE, AEP_MUTAGENIC, AEP_SICK, AEP_NULL},
     {AEA_ADRENALINE, AEA_BLOOD, AEA_HEAL, AEA_BUGS},
     {AEA_RADIATION, AEA_PAIN, AEA_PAIN, AEA_VOMIT}
    },
    {_("glittering"), _("glitters faintly under direct light"),
     {AEP_INT_UP, AEP_EXTINGUISH, AEP_GLOW, AEP_NULL},
     {AEP_SMOKE, AEP_ATTENTION, AEP_NULL, AEP_NULL},
     {AEA_MAP, AEA_LIGHT, AEA_CONFUSED, AEA_ENTRANCE},
     {AEA_RADIATION, AEA_MUTATE, AEA_ATTENTION, AEA_FLASH}
    },
    {_("electric"), _("very weakly shocks you when touched"),
     {AEP_RESIST_ELECTRICITY, AEP_DEX_UP, AEP_SPEED_UP, AEP_PSYSHIELD},
     {AEP_THIRST, AEP_SMOKE, AEP_STR_DOWN, AEP_BAD_WEATHER},
     {AEA_STORM, AEA_ADRENALINE, AEA_LIGHT, AEA_NULL},
     {AEA_PAIN, AEA_PARALYZE, AEA_FLASH, AEA_FLASH}
    },
    {_("slimy"), _("feels slimy"),
     {AEP_SNAKES, AEP_STEALTH, AEP_EXTINGUISH, AEP_SAP_LIFE},
     {AEP_THIRST, AEP_DEX_DOWN, AEP_SPEED_DOWN, AEP_SICK},
     {AEA_BLOOD, AEA_ACIDBALL, AEA_GROWTH, AEA_ACIDBALL},
     {AEA_MUTATE, AEA_MUTATE, AEA_VOMIT, AEA_VOMIT}
    },
    {_("engraved"), _("is covered with odd etchings"),
     {AEP_CLAIRVOYANCE, AEP_INVISIBLE, AEP_PSYSHIELD, AEP_SAP_LIFE},
     {AEP_EVIL, AEP_ATTENTION, AEP_NULL, AEP_NULL},
     {AEA_FATIGUE, AEA_TELEPORT, AEA_HEAL, AEA_FATIGUE},
     {AEA_ATTENTION, AEA_ATTENTION, AEA_TELEGLOW, AEA_DIM}
    },
    {_("crackling"), _("occasionally makes a soft crackling sound"),
     {AEP_EXTINGUISH, AEP_RESIST_ELECTRICITY, AEP_NULL, AEP_NULL},
     {AEP_SMOKE, AEP_RADIOACTIVE, AEP_MOVEMENT_NOISE, AEP_NULL},
     {AEA_STORM, AEA_FIREBALL, AEA_PULSE, AEA_NULL},
     {AEA_PAIN, AEA_PARALYZE, AEA_NOISE, AEA_NOISE}
    },
    {_("warm"), _("is warm to the touch"),
     {AEP_STR_UP, AEP_EXTINGUISH, AEP_GLOW, AEP_NULL},
     {AEP_SMOKE, AEP_RADIOACTIVE, AEP_NULL, AEP_NULL},
     {AEA_FIREBALL, AEA_FIREBALL, AEA_FIREBALL, AEA_LIGHT},
     {AEA_FIRESTORM, AEA_FIRESTORM, AEA_TELEGLOW, AEA_NULL}
    },
    {_("rattling"), _("makes a rattling sound when moved"),
     {AEP_DEX_UP, AEP_SPEED_UP, AEP_SNAKES, AEP_CARRY_MORE},
     {AEP_ATTENTION, AEP_INT_DOWN, AEP_MOVEMENT_NOISE, AEP_MOVEMENT_NOISE},
     {AEA_BLOOD, AEA_PULSE, AEA_BUGS, AEA_NULL},
     {AEA_PAIN, AEA_ATTENTION, AEA_NOISE, AEA_NULL}
    },
    {_("scaled"), _("has a surface reminiscent of reptile scales"),
     {AEP_SNAKES, AEP_SNAKES, AEP_SNAKES, AEP_STEALTH},
     {AEP_THIRST, AEP_MUTAGENIC, AEP_SPEED_DOWN, AEP_NULL},
     {AEA_ADRENALINE, AEA_BUGS, AEA_GROWTH, AEA_NULL},
     {AEA_MUTATE, AEA_SCREAM, AEA_DIM, AEA_NULL}
    },
    {_("fractal"),
    _("has a self-similar pattern which repeats until it is too small for you to see"),
     {AEP_ALL_UP, AEP_ALL_UP, AEP_CLAIRVOYANCE, AEP_PSYSHIELD},
     {AEP_SCHIZO, AEP_ATTENTION, AEP_FORCE_TELEPORT, AEP_BAD_WEATHER},
     {AEA_STORM, AEA_FATIGUE, AEA_TELEPORT, AEA_NULL},
     {AEA_RADIATION, AEA_MUTATE, AEA_TELEGLOW, AEA_TELEGLOW}
    }
    };
    for(int i=0;i<ARTPROP_MAX;i++) {artifact_property_data[i]=tmp_artifact_property_data[i];}

    artifact_tool_form_datum tmp_artifact_tool_form_data[NUM_ARTTOOLFORMS] = {
    {"", '*', c_white, "null", "null", 0, 0, 0, 0, ARTWEAP_BULK,
     {ARTWEAP_NULL, ARTWEAP_NULL, ARTWEAP_NULL}},

    {_("Harp"), ';', c_yellow, "wood", "null", 20, 30, 1150, 2100, ARTWEAP_BULK,
     {ARTWEAP_SPEAR, ARTWEAP_SWORD, ARTWEAP_KNIFE}},

    {_("Staff"), '/', c_brown, "wood", "null", 6, 12, 450, 1150, ARTWEAP_CLUB,
     {ARTWEAP_BULK, ARTWEAP_SPEAR, ARTWEAP_KNIFE}},

    {_("Sword"), '/', c_ltblue, "steel", "null", 8, 14, 900, 3259, ARTWEAP_SWORD,
     {ARTWEAP_BULK, ARTWEAP_NULL, ARTWEAP_NULL}},

    {_("Dagger"), ';', c_ltblue, "steel", "null", 1, 4, 100, 700, ARTWEAP_KNIFE,
     {ARTWEAP_NULL, ARTWEAP_NULL, ARTWEAP_NULL}},

    {_("Cube"), '*', c_white, "steel", "null", 1, 3, 100, 2300, ARTWEAP_BULK,
     {ARTWEAP_SPEAR, ARTWEAP_NULL, ARTWEAP_NULL}}
    };
    for(int i=0;i<NUM_ARTTOOLFORMS;i++) {artifact_tool_form_data[i]=tmp_artifact_tool_form_data[i];}

    artifact_weapon_datum tmp_artifact_weapon_data[NUM_ARTWEAPS] = {
        {"", 0, 0, 0, 0, 0, 0, 0, 0, ""},
        // Adjective     Vol,wgt   Bash       Cut     To-Hit   tags
        {_("Heavy"),     0,1400,  10, 20,    0,  0,   -2,  0,  ""},
        {_("Knobbed"),   1,250,   14, 30,    0,  0,   -1,  1,  ""},
        {_("Spiked"),    1,100,    0,  0,   20, 40,   -1,  1,  "SPEAR"},
        {_("Edged"),     2,450,    0,  0,   20, 50,   -1,  2,  "CHOP"},
        {_("Bladed"),    1,2250,   0,  0,   12, 30,   -1,  1,  "STAB"}
    };
    for(int i=0;i<NUM_ARTWEAPS;i++) {artifact_weapon_data[i]=tmp_artifact_weapon_data[i];}

    artifact_armor_form_datum tmp_artifact_armor_form_data[NUM_ARTARMFORMS] = {
    {"", c_white, "null", "null",        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0, false,
     {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}},
    // Name    color  Materials         Vol Wgt Enc Cov Thk Env Wrm Sto Bsh Cut Hit
    {_("Robe"),   c_red, "wool", "null", 6, 700,  1,  3,  3,  0,  2,  0, -8,  0, -3,
     mfb(bp_torso)|mfb(bp_legs), false,
     {ARMORMOD_LIGHT, ARMORMOD_BULKY, ARMORMOD_POCKETED, ARMORMOD_FURRED,
      ARMORMOD_PADDED}},

    {_("Coat"),   c_brown,"leather","null", 14, 1600,  2,  3, 2,  1,  4,  4, -6,  0, -3,
     mfb(bp_torso), false,
     {ARMORMOD_LIGHT, ARMORMOD_POCKETED, ARMORMOD_FURRED, ARMORMOD_PADDED,
      ARMORMOD_PLATED}},

    {_("Mask"),   c_white, "wood", "null",   4, 100,  2,  2, 2,  1,  2,  0,  2,  0, -2,
     mfb(bp_eyes)|mfb(bp_mouth), false,
     {ARMORMOD_FURRED, ARMORMOD_FURRED, ARMORMOD_NULL, ARMORMOD_NULL,
      ARMORMOD_NULL}},

    // Name    color  Materials             Vol  Wgt Enc Cov Thk Env Wrm Sto Bsh Cut Hit
    {_("Helm"),   c_dkgray, "silver", "null", 6, 700,  2,  3, 3,  0,  1,  0,  8,  0, -2,
     mfb(bp_head), false,
     {ARMORMOD_BULKY, ARMORMOD_FURRED, ARMORMOD_PADDED, ARMORMOD_PLATED,
      ARMORMOD_NULL}},

    {_("Gloves"), c_ltblue,"leather","null", 2, 100,  1,  3,  3,  1,  2,  0, -4,  0, -2,
     mfb(bp_hands), true,
     {ARMORMOD_BULKY, ARMORMOD_FURRED, ARMORMOD_PADDED, ARMORMOD_PLATED,
      ARMORMOD_NULL}},

    // Name    color  Materials            Vol  Wgt Enc Cov Thk Env Wrm Sto Bsh Cut Hit
    {_("Boots"), c_blue, "leather", "null",  6, 250,  1,  3,  3,  1,  3,  0,  4,  0, -1,
     mfb(bp_feet), true,
     {ARMORMOD_LIGHT, ARMORMOD_BULKY, ARMORMOD_PADDED, ARMORMOD_PLATED,
      ARMORMOD_NULL}},

    {_("Ring"), c_ltgreen, "silver", "null",   0,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0, true,
     {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}}

    };
    for(int i=0;i<NUM_ARTARMFORMS;i++) {artifact_armor_form_data[i]=tmp_artifact_armor_form_data[i];}

    artifact_armor_form_datum tmp_artifact_armor_mod_data[NUM_ARMORMODS] = {

    {"", c_white, "null", "null", 0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, false,
     {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}},
    // Description; "It is ..." or "They are ..."
    {_("very thin and light."), c_white, "null", "null",
    // Vol   Wgt Enc Cov Thk Env Wrm Sto
        -4, -950, -2, -1, -1, -1, -1,  0, 0, 0, 0, 0,  false,
     {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}},

    {_("extremely bulky."), c_white, "null", "null",
         8, 1150,  2,  1,  1,  0,  1,  0, 0, 0, 0, 0,  false,
     {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}},

    {_("covered in pockets."), c_white, "null", "null",
         1, 150,  1,  0,  0,  0,  0, 16, 0, 0, 0, 0,  false,
     {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}},

    {_("disgustingly furry."), c_white, "wool", "null",
    // Vol  Wgt Enc Dmg Cut Env Wrm Sto
         4, 250,  1,  1,  1,  1,  3,  0, 0, 0, 0, 0,  false,
     {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}},

    {_("leather-padded."), c_white, "leather", "null",
         4, 450,  1, 1,  1,  0,  1, -3, 0, 0, 0, 0,  false,
     {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}},

    {_("plated in iron."), c_white, "iron", "null",
         4, 1400,  3,  2, 2,  0,  1, -4, 0, 0, 0, 0, false,
     {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}},

    };
    for(int i=0;i<NUM_ARMORMODS;i++) {artifact_armor_mod_data[i]=tmp_artifact_armor_mod_data[i];}

    std::string tmp_artifact_adj[NUM_ART_ADJS] = {
    _("Forbidden"), _("Unknown"), _("Forgotten"), _("Hideous"), _("Eldritch"),
    _("Gelatinous"), _("Ancient"), _("Cursed"), _("Bloody"), _("Undying"),
    _("Shadowy"), _("Silent"), _("Cyclopean"), _("Fungal"), _("Unspeakable"),
    _("Grotesque"), _("Frigid"), _("Shattered"), _("Sleeping"), _("Repellent")
    };
    for(int i=0;i<NUM_ART_ADJS;i++) {artifact_adj[i]=tmp_artifact_adj[i];}

    std::string tmp_artifact_noun[NUM_ART_NOUNS] = {
    _("%s Technique"), _("%s Dreams"), _("%s Beasts"), _("%s Evil"), _("%s Miasma"),
    _("the %s Abyss"), _("the %s City"), _("%s Shadows"), _("%s Shade"), _("%s Illusion"),
    _("%s Justice"), _("the %s Necropolis"), _("%s Ichor"), _("the %s Monolith"), _("%s Aeons"),
    _("%s Graves"), _("%s Horrors"), _("%s Suffering"), _("%s Death"), _("%s Horror")
    };
    for(int i=0;i<NUM_ART_NOUNS;i++) {artifact_noun[i]=tmp_artifact_noun[i];}

}

itype* new_artifact(itypemap &itypes)
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
  if( weapon->tag != "" ) {
      art->item_tags.insert(weapon->tag);
  }
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
    if( weapon->tag != "" ) {
        art->item_tags.insert(weapon->tag);
    }
    std::stringstream newname;
    newname << weapon->adjective << " " << info->name;
    art->name = artifact_name(newname.str());
   }
  }
  art->description = string_format(_("This is the %s.\nIt is the only one of its kind.\nIt may have unknown powers; use 'a' to activate them."), art->name.c_str());

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
  art->rand_charges.push_back(art->max_charges);
// If we have charges, pick a recharge mechanism
  if (art->max_charges > 0)
   art->charge_type = art_charge( rng(ARTC_NULL + 1, NUM_ARTCS - 1) );
  if (one_in(8) && num_bad + num_good >= 4)
   art->charge_type = ARTC_NULL; // 1 in 8 chance that it can't recharge!

  itypes[art->id]=art;
  artifact_itype_ids.push_back(art->id);
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
  art->covers = info->covers;
  art->encumber = info->encumb;
  art->coverage = info->coverage;
  art->thickness = info->thickness;
  art->env_resist = info->env_resist;
  art->warmth = info->warmth;
  art->storage = info->storage;
  std::stringstream description;
  description << string_format(info->plural?
    _("This is the %s.\nThey are the only ones of their kind.") :
    _("This is the %s.\nIt is the only one of its kind."),
    art->name.c_str());

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

    if (modinfo->coverage > 0 || art->coverage > abs(modinfo->coverage))
     art->coverage += modinfo->coverage;
    else
     art->coverage = 0;

    if (modinfo->thickness > 0 || art->thickness > abs(modinfo->thickness))
     art->thickness += modinfo->thickness;
    else
     art->thickness = 0;

    if (modinfo->env_resist > 0 || art->env_resist > abs(modinfo->env_resist))
     art->env_resist += modinfo->env_resist;
    else
     art->env_resist = 0;
    art->warmth += modinfo->warmth;

    if (modinfo->storage > 0 || art->storage > abs(modinfo->storage))
     art->storage += modinfo->storage;
    else
     art->storage = 0;

    description << string_format(info->plural?
        _("\nThey are %s") :
        _("\nIt is %s"),
        modinfo->name.c_str());
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
  itypes[art->id] = art;
  artifact_itype_ids.push_back(art->id);
  return art;
 }
}

itype* new_natural_artifact(itypemap &itypes, artifact_natural_property prop)
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
 art->m1 = "stone";
 art->m2 = "null";
 art->volume = rng(shape_data->volume_min, shape_data->volume_max);
 art->weight = rng(shape_data->weight_min, shape_data->weight_max);
 art->melee_dam = 0;
 art->melee_cut = 0;
 art->m_to_hit = 0;

 art->name = rmp_format(_("<artifact_name>%1$s %2$s"), property_data->name.c_str(), shape_data->name.c_str());
 art->description = rmp_format(_("<artifact_desc>This %1$s %2$s."), shape_data->desc.c_str(), property_data->desc.c_str());

// Add line breaks to the description as necessary
/*
 size_t pos = 76;
 while (art->description.length() - pos >= 76) {
  pos = art->description.find_last_of(' ', pos);
  if (pos == std::string::npos)
   pos = art->description.length();
  else {
   art->description[pos] = '\n';
   pos += 76;
  }
 }*/

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
  art->rand_charges.push_back(art->max_charges);
  art->charge_type = art_charge( rng(ARTC_NULL + 1, NUM_ARTCS - 1) );
 }
 artifact_itype_ids.push_back(art->id);
 itypes[art->id] = art;
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
 std::string ret;
 const char *fmtstr = _("<artifact_name>%1$s of %2$s");
 std::string noun = artifact_noun[rng(0, NUM_ART_NOUNS - 1)];
 std::string adj = artifact_adj[rng(0, NUM_ART_ADJS - 1)];
 ret = string_format(noun, adj.c_str());
 ret = rmp_format(fmtstr, type.c_str(),ret.c_str());
 return ret;
}


/* Json Loading and saving */

void load_artifacts(const std::string &artfilename, itypemap &itypes)
{
    std::ifstream file_test(artfilename.c_str(),
                            std::ifstream::in | std::ifstream::binary);
    if (!file_test.good()) {
        file_test.close();
        return;
    }

    try {
        load_artifacts_from_ifstream(file_test, itypes);
    } catch (std::string e) {
        debugmsg("%s: %s", artfilename.c_str(), e.c_str());
    }

    file_test.close();
}

void load_artifacts_from_ifstream(std::ifstream &f, itypemap &itypes)
{
    // delete current artefact ids
    artifact_itype_ids.clear();
    // read and create artifacts from json array in artifacts.gsav
    JsonIn artifact_json(f);
    artifact_json.start_array();
    while (!artifact_json.end_array()) {
        JsonObject jo = artifact_json.get_object();
        std::string type = jo.get_string("type");
        std::string id = jo.get_string("id");
        if (type == "artifact_tool") {
            it_artifact_tool *art = new it_artifact_tool(jo);
            itypes[id] = art;
            artifact_itype_ids.push_back(id);
        } else if (type == "artifact_armor") {
            it_artifact_armor *art = new it_artifact_armor(jo);
            itypes[id] = art;
            artifact_itype_ids.push_back(id);
        } else {
            throw jo.line_number() + ": unrecognized artifact type.";
        }
    }
}


void it_artifact_tool::deserialize(JsonObject &jo)
{
    id = jo.get_string("id");
    name = jo.get_string("name");
    description = jo.get_string("description");
    sym = jo.get_int("sym");
    color = int_to_color(jo.get_int("color"));
    price = jo.get_int("price");
    m1 = jo.get_string("m1");
    m2 = jo.get_string("m2");
    volume = jo.get_int("volume");
    weight = jo.get_int("weight");
    melee_dam = jo.get_int("melee_dam");
    melee_cut = jo.get_int("melee_cut");
    m_to_hit = jo.get_int("m_to_hit");
    item_tags = jo.get_tags("item_flags");

    max_charges = jo.get_int("max_charges");
    def_charges = jo.get_int("def_charges");

    std::vector<int> rand_charges;
    JsonArray jarr = jo.get_array("rand_charges");
    while (jarr.has_more()){
        rand_charges.push_back(jarr.next_int());
    }

    charges_per_use = jo.get_int("charges_per_use");
    turns_per_charge = jo.get_int("turns_per_charge");
    ammo = jo.get_string("ammo");
    revert_to = jo.get_string("revert_to");

    charge_type = (art_charge)jo.get_int("charge_type");

    JsonArray ja = jo.get_array("effects_wielded");
    while (ja.has_more()) {
        effects_wielded.push_back((art_effect_passive)ja.next_int());
    }

    ja = jo.get_array("effects_activated");
    while (ja.has_more()) {
        effects_activated.push_back((art_effect_active)ja.next_int());
    }

    ja = jo.get_array("effects_carried");
    while (ja.has_more()) {
        effects_carried.push_back((art_effect_passive)ja.next_int());
    }
}

void it_artifact_armor::deserialize(JsonObject &jo)
{
    id = jo.get_string("id");
    name = jo.get_string("name");
    description = jo.get_string("description");
    sym = jo.get_int("sym");
    color = int_to_color(jo.get_int("color"));
    price = jo.get_int("price");
    m1 = jo.get_string("m1");
    m2 = jo.get_string("m2");
    volume = jo.get_int("volume");
    weight = jo.get_int("weight");
    melee_dam = jo.get_int("melee_dam");
    melee_cut = jo.get_int("melee_cut");
    m_to_hit = jo.get_int("m_to_hit");
    item_tags = jo.get_tags("item_flags");

    covers = jo.get_int("covers");
    encumber = jo.get_int("encumber");
    coverage = jo.get_int("coverage");
    thickness = jo.get_int("material_thickness");
    env_resist = jo.get_int("env_resist");
    warmth = jo.get_int("warmth");
    storage = jo.get_int("storage");
    power_armor = jo.get_bool("power_armor");

    JsonArray ja = jo.get_array("effects_worn");
    while (ja.has_more()) {
        effects_worn.push_back((art_effect_passive)ja.next_int());
    }
}

void it_artifact_tool::serialize(JsonOut &json) const
{
    json.start_object();

    json.member("type", "artifact_tool");

    // generic data
    json.member("id", id);
    json.member("name", name);
    json.member("description", description);
    json.member("sym", sym);
    json.member("color", color_to_int(color));
    json.member("price", price);
    json.member("m1", m1);
    json.member("m2", m2);
    json.member("volume", volume);
    json.member("weight", weight);
    json.member("melee_dam", melee_dam);
    json.member("melee_cut", melee_cut);
    json.member("m_to_hit", m_to_hit);

    json.member("item_flags", item_tags);
    json.member("techniques", techniques);

    // tool data
    json.member("ammo", ammo);
    json.member("max_charges", max_charges);
    json.member("def_charges", def_charges);
    json.member("rand_charges", rand_charges);
    json.member("charges_per_use", charges_per_use);
    json.member("turns_per_charge", turns_per_charge);
    json.member("revert_to", revert_to);

    // artifact data
    json.member("charge_type", charge_type);
    json.member("effects_wielded", effects_wielded);
    json.member("effects_activated", effects_activated);
    json.member("effects_carried", effects_carried);

    json.end_object();
}

void it_artifact_armor::serialize(JsonOut &json) const
{
    json.start_object();

    json.member("type", "artifact_armor");

    // generic data
    json.member("id", id);
    json.member("name", name);
    json.member("description", description);
    json.member("sym", sym);
    json.member("color", color_to_int(color));
    json.member("price", price);
    json.member("m1", m1);
    json.member("m2", m2);
    json.member("volume", volume);
    json.member("weight", weight);
    json.member("id", id);
    json.member("melee_dam", melee_dam);
    json.member("melee_cut", melee_cut);
    json.member("m_to_hit", m_to_hit);

    json.member("item_flags", item_tags);

    json.member("techniques", techniques);

    // armor data
    json.member("covers", covers);
    json.member("encumber", encumber);
    json.member("coverage", coverage);
    json.member("material_thickness", thickness);
    json.member("env_resist", env_resist);
    json.member("warmth", warmth);
    json.member("storage", storage);
    json.member("power_armor", power_armor);

    // artifact data
    json.member("effects_worn", effects_worn);

    json.end_object();
}

