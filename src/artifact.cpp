#include "artifact.h"

#include "output.h" // string_format
#include "item_factory.h"
#include "debug.h"
#include "json.h"
#include "cata_utility.h"
#include "rng.h"
#include "translations.h"

#include <bitset>
#include <cmath>
#include <sstream>

// mfb(t_flag) converts a flag to a bit for insertion into a bitfield
#ifndef mfb
#define mfb(n) static_cast <unsigned long> (1 << (n))
#endif

template<typename V, typename B>
inline units::quantity<V, B> rng( const units::quantity<V, B> &min, const units::quantity<V, B> &max )
{
    return units::quantity<V, B>( rng( min.value(), max.value() ), B{} );
}

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
    2, // AEP_PBLUE
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

enum artifact_natural_shape {
    ARTSHAPE_NULL,
    ARTSHAPE_SPHERE,
    ARTSHAPE_ROD,
    ARTSHAPE_TEARDROP,
    ARTSHAPE_LAMP,
    ARTSHAPE_SNAKE,
    ARTSHAPE_DISC,
    ARTSHAPE_BEADS,
    ARTSHAPE_NAPKIN,
    ARTSHAPE_URCHIN,
    ARTSHAPE_JELLY,
    ARTSHAPE_SPIRAL,
    ARTSHAPE_PIN,
    ARTSHAPE_TUBE,
    ARTSHAPE_PYRAMID,
    ARTSHAPE_CRYSTAL,
    ARTSHAPE_KNOT,
    ARTSHAPE_CRESCENT,
    ARTSHAPE_MAX
};

struct artifact_shape_datum {
    std::string name;
    std::string desc;
    units::volume volume_min, volume_max;
    int weight_min, weight_max;
};

struct artifact_property_datum {
    std::string name;
    std::string desc;
    art_effect_passive passive_good[4];
    art_effect_passive passive_bad[4];
    art_effect_active active_good[4];
    art_effect_active active_bad[4];
};

enum artifact_weapon_type {
    ARTWEAP_NULL,
    ARTWEAP_BULK,  // A bulky item that works okay for bashing
    ARTWEAP_CLUB,  // An item designed to bash
    ARTWEAP_SPEAR, // A stab-only weapon
    ARTWEAP_SWORD, // A long slasher
    ARTWEAP_KNIFE, // Short, slash and stab
    NUM_ARTWEAPS
};

struct artifact_tool_form_datum {
    std::string name;
    char sym;
    nc_color color;
    // Most things had 0 to 1 material.
    material_id material;
    units::volume volume_min, volume_max;
    int weight_min, weight_max;
    artifact_weapon_type base_weapon;
    artifact_weapon_type extra_weapons[3];
};

enum artifact_tool_form {
    ARTTOOLFORM_NULL,
    ARTTOOLFORM_HARP,
    ARTTOOLFORM_STAFF,
    ARTTOOLFORM_SWORD,
    ARTTOOLFORM_KNIFE,
    ARTTOOLFORM_CUBE,
    NUM_ARTTOOLFORMS
};

struct artifact_weapon_datum {
    std::string adjective;
    units::volume volume;
    int weight; // Only applicable if this is an *extra* weapon
    int bash_min, bash_max;
    int cut_min, cut_max;
    int stab_min, stab_max;
    int to_hit_min, to_hit_max;
    std::string tag;
};

enum artifact_armor_mod {
    ARMORMOD_NULL,
    ARMORMOD_LIGHT,
    ARMORMOD_BULKY,
    ARMORMOD_POCKETED,
    ARMORMOD_FURRED,
    ARMORMOD_PADDED,
    ARMORMOD_PLATED,
    NUM_ARMORMODS
};

struct artifact_armor_form_datum {
    std::string name;
    nc_color color;
    // Most things had 0 to 1 material.
    material_id material;
    units::volume volume;
    int weight;
    int encumb;
    int coverage;
    int thickness;
    int env_resist;
    int warmth;
    units::volume storage;
    int melee_bash, melee_cut, melee_hit;
    std::bitset<num_bp> covers;
    bool plural;
    artifact_armor_mod available_mods[5];
};

enum artifact_armor_form {
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

//see below, move them so gettext and be applied properly
artifact_shape_datum artifact_shape_data[ARTSHAPE_MAX];
artifact_property_datum artifact_property_data[ARTPROP_MAX];
artifact_tool_form_datum artifact_tool_form_data[NUM_ARTTOOLFORMS];
artifact_weapon_datum artifact_weapon_data[NUM_ARTWEAPS];
artifact_armor_form_datum artifact_armor_form_data[NUM_ARTARMFORMS];
/*
 * Armor mods alter the normal values of armor.
 * If the basic armor type has "null" as its second material, and the mod has a
 * material attached, the second material will be changed.
 */
artifact_armor_form_datum artifact_armor_mod_data[NUM_ARMORMODS];
#define NUM_ART_ADJS 20
std::string artifact_adj[NUM_ART_ADJS];
#define NUM_ART_NOUNS 20
std::string artifact_noun[NUM_ART_NOUNS];
std::string artifact_name(std::string type);

// Constructrs for artifact itypes.
it_artifact_tool::it_artifact_tool() : itype()
{
    tool.reset( new islot_tool() );
    artifact.reset( new islot_artifact() );
    id = item_controller->create_artifact_id();
    price = 0;
    tool->charges_per_use = 1;
    artifact->charge_type = ARTC_NULL;
    use_methods.emplace( "ARTIFACT", use_function( "ARTIFACT", &iuse::artifact ) );
}

it_artifact_tool::it_artifact_tool( JsonObject &jo ) : itype()
{
    tool.reset( new islot_tool() );
    artifact.reset( new islot_artifact() );
    use_methods.emplace( "ARTIFACT", use_function( "ARTIFACT", &iuse::artifact ) );
    deserialize( jo );
}

it_artifact_armor::it_artifact_armor() : itype()
{
    armor.reset( new islot_armor() );
    artifact.reset( new islot_artifact() );
    id = item_controller->create_artifact_id();
    price = 0;
}

it_artifact_armor::it_artifact_armor( JsonObject &jo ) : itype()
{
    armor.reset( new islot_armor() );
    artifact.reset( new islot_artifact() );
    deserialize( jo );
}

void init_artifacts()
{
    artifact_shape_datum tmp_artifact_shape_data[ARTSHAPE_MAX] = {
        {"BUG", "BUG", 0_ml, 0_ml, 0, 0},
        {_("sphere"), _("smooth sphere"), 500_ml, 1000_ml, 1, 1150},
        {_("rod"), _("tapered rod"), 250_ml, 1750_ml, 1, 800},
        {_("teardrop"), _("teardrop-shaped stone"), 500_ml, 1500_ml, 1, 950},
        {_("lamp"), _("hollow, transparent cube"), 1000_ml, 225_ml, 1, 350},
        {_("snake"), _("winding, flexible rod"), 0_ml, 2000_ml, 1, 950},
        {_("disc"), _("smooth disc"), 1000_ml, 1500_ml, 200, 400},
        {_("beads"), _("string of beads"), 750_ml, 1750_ml, 1, 700},
        {_("napkin"), _("very thin sheet"), 0_ml, 750_ml, 1, 350},
        {_("urchin"), _("spiked sphere"), 750_ml, 1250_ml, 200, 700},
        {_("jelly"), _("malleable blob"), 500_ml, 2000_ml, 200, 450},
        {_("spiral"), _("spiraling rod"), 1250_ml, 1500_ml, 200, 350},
        {_("pin"), _("pointed rod"), 250_ml, 1250_ml, 100, 1050},
        {_("tube"), _("hollow tube"), 500_ml, 1250_ml, 350, 700},
        {_("pyramid"), _("regular tetrahedron"), 750_ml, 1750_ml, 200, 450},
        {_("crystal"), _("translucent crystal"), 250_ml, 1500_ml, 200, 800},
        {_("knot"), _("twisted, knotted cord"), 500_ml, 1500_ml, 100, 800},
        {_("crescent"), _("crescent-shaped stone"), 500_ml, 1500_ml, 200, 700}
    };
    for(int i = 0; i < ARTSHAPE_MAX; i++) {
        artifact_shape_data[i] = tmp_artifact_shape_data[i];
    }

    artifact_property_datum tmp_artifact_property_data[ARTPROP_MAX] = {
        {
            "BUG", "BUG",
            {AEP_NULL, AEP_NULL, AEP_NULL, AEP_NULL},
            {AEP_NULL, AEP_NULL, AEP_NULL, AEP_NULL},
            {AEA_NULL, AEA_NULL, AEA_NULL, AEA_NULL},
            {AEA_NULL, AEA_NULL, AEA_NULL, AEA_NULL}
        },
        {
            _("wriggling"), _("is constantly wriggling"),
            {AEP_SPEED_UP, AEP_SNAKES, AEP_NULL, AEP_NULL},
            {AEP_DEX_DOWN, AEP_FORCE_TELEPORT, AEP_SICK, AEP_NULL},
            {AEA_TELEPORT, AEA_ADRENALINE, AEA_NULL, AEA_NULL},
            {AEA_MUTATE, AEA_ATTENTION, AEA_VOMIT, AEA_NULL}
        },
        {
            _("glowing"), _("glows faintly"),
            {AEP_INT_UP, AEP_GLOW, AEP_CLAIRVOYANCE, AEP_NULL},
            {AEP_RADIOACTIVE, AEP_MUTAGENIC, AEP_ATTENTION, AEP_NULL},
            {AEA_LIGHT, AEA_LIGHT, AEA_LIGHT, AEA_NULL},
            {AEA_ATTENTION, AEA_TELEGLOW, AEA_FLASH, AEA_SHADOWS}
        },
        {
            _("humming"), _("hums very quietly"),
            {AEP_ALL_UP, AEP_PSYSHIELD, AEP_NULL, AEP_NULL},
            {AEP_SCHIZO, AEP_PER_DOWN, AEP_INT_DOWN, AEP_NULL},
            {AEA_PULSE, AEA_ENTRANCE, AEA_NULL, AEA_NULL},
            {AEA_NOISE, AEA_NOISE, AEA_SCREAM, AEA_NULL}
        },
        {
            _("moving"), _("shifts from side to side slowly"),
            {AEP_STR_UP, AEP_DEX_UP, AEP_SPEED_UP, AEP_NULL},
            {AEP_HUNGER, AEP_PER_DOWN, AEP_FORCE_TELEPORT, AEP_NULL},
            {AEA_TELEPORT, AEA_TELEPORT, AEA_MAP, AEA_NULL},
            {AEA_PARALYZE, AEA_VOMIT, AEA_VOMIT, AEA_NULL}
        },
        {
            _("whispering"), _("makes very faint whispering sounds"),
            {AEP_CLAIRVOYANCE, AEP_EXTINGUISH, AEP_STEALTH, AEP_NULL},
            {AEP_EVIL, AEP_SCHIZO, AEP_ATTENTION, AEP_NULL},
            {AEA_FATIGUE, AEA_ENTRANCE, AEA_ENTRANCE, AEA_NULL},
            {AEA_ATTENTION, AEA_SCREAM, AEA_SCREAM, AEA_SHADOWS}
        },
        {
            _("breathing"),
            _("shrinks and grows very slightly with a regular pulse, as if breathing"),
            {AEP_SAP_LIFE, AEP_ALL_UP, AEP_SPEED_UP, AEP_CARRY_MORE},
            {AEP_HUNGER, AEP_THIRST, AEP_SICK, AEP_BAD_WEATHER},
            {AEA_ADRENALINE, AEA_HEAL, AEA_ENTRANCE, AEA_GROWTH},
            {AEA_MUTATE, AEA_ATTENTION, AEA_SHADOWS, AEA_NULL}
        },
        {
            _("dead"), _("is icy cold to the touch"),
            {AEP_INVISIBLE, AEP_CLAIRVOYANCE, AEP_EXTINGUISH, AEP_SAP_LIFE},
            {AEP_HUNGER, AEP_EVIL, AEP_ALL_DOWN, AEP_SICK},
            {AEA_BLOOD, AEA_HURTALL, AEA_NULL, AEA_NULL},
            {AEA_PAIN, AEA_SHADOWS, AEA_DIM, AEA_VOMIT}
        },
        {
            _("itchy"), _("makes your skin itch slightly when it is close"),
            {AEP_DEX_UP, AEP_SPEED_UP, AEP_PSYSHIELD, AEP_NULL},
            {AEP_RADIOACTIVE, AEP_MUTAGENIC, AEP_SICK, AEP_NULL},
            {AEA_ADRENALINE, AEA_BLOOD, AEA_HEAL, AEA_BUGS},
            {AEA_RADIATION, AEA_PAIN, AEA_PAIN, AEA_VOMIT}
        },
        {
            _("glittering"), _("glitters faintly under direct light"),
            {AEP_INT_UP, AEP_EXTINGUISH, AEP_GLOW, AEP_NULL},
            {AEP_SMOKE, AEP_ATTENTION, AEP_NULL, AEP_NULL},
            {AEA_MAP, AEA_LIGHT, AEA_CONFUSED, AEA_ENTRANCE},
            {AEA_RADIATION, AEA_MUTATE, AEA_ATTENTION, AEA_FLASH}
        },
        {
            _("electric"), _("very weakly shocks you when touched"),
            {AEP_RESIST_ELECTRICITY, AEP_DEX_UP, AEP_SPEED_UP, AEP_PSYSHIELD},
            {AEP_THIRST, AEP_SMOKE, AEP_STR_DOWN, AEP_BAD_WEATHER},
            {AEA_STORM, AEA_ADRENALINE, AEA_LIGHT, AEA_NULL},
            {AEA_PAIN, AEA_PARALYZE, AEA_FLASH, AEA_FLASH}
        },
        {
            _("slimy"), _("feels slimy"),
            {AEP_SNAKES, AEP_STEALTH, AEP_EXTINGUISH, AEP_SAP_LIFE},
            {AEP_THIRST, AEP_DEX_DOWN, AEP_SPEED_DOWN, AEP_SICK},
            {AEA_BLOOD, AEA_ACIDBALL, AEA_GROWTH, AEA_ACIDBALL},
            {AEA_MUTATE, AEA_MUTATE, AEA_VOMIT, AEA_VOMIT}
        },
        {
            _("engraved"), _("is covered with odd etchings"),
            {AEP_CLAIRVOYANCE, AEP_INVISIBLE, AEP_PSYSHIELD, AEP_SAP_LIFE},
            {AEP_EVIL, AEP_ATTENTION, AEP_NULL, AEP_NULL},
            {AEA_FATIGUE, AEA_TELEPORT, AEA_HEAL, AEA_FATIGUE},
            {AEA_ATTENTION, AEA_ATTENTION, AEA_TELEGLOW, AEA_DIM}
        },
        {
            _("crackling"), _("occasionally makes a soft crackling sound"),
            {AEP_EXTINGUISH, AEP_RESIST_ELECTRICITY, AEP_NULL, AEP_NULL},
            {AEP_SMOKE, AEP_RADIOACTIVE, AEP_MOVEMENT_NOISE, AEP_NULL},
            {AEA_STORM, AEA_FIREBALL, AEA_PULSE, AEA_NULL},
            {AEA_PAIN, AEA_PARALYZE, AEA_NOISE, AEA_NOISE}
        },
        {
            _("warm"), _("is warm to the touch"),
            {AEP_STR_UP, AEP_EXTINGUISH, AEP_GLOW, AEP_NULL},
            {AEP_SMOKE, AEP_RADIOACTIVE, AEP_NULL, AEP_NULL},
            {AEA_FIREBALL, AEA_FIREBALL, AEA_FIREBALL, AEA_LIGHT},
            {AEA_FIRESTORM, AEA_FIRESTORM, AEA_TELEGLOW, AEA_NULL}
        },
        {
            _("rattling"), _("makes a rattling sound when moved"),
            {AEP_DEX_UP, AEP_SPEED_UP, AEP_SNAKES, AEP_CARRY_MORE},
            {AEP_ATTENTION, AEP_INT_DOWN, AEP_MOVEMENT_NOISE, AEP_MOVEMENT_NOISE},
            {AEA_BLOOD, AEA_PULSE, AEA_BUGS, AEA_NULL},
            {AEA_PAIN, AEA_ATTENTION, AEA_NOISE, AEA_NULL}
        },
        {
            _("scaled"), _("has a surface reminiscent of reptile scales"),
            {AEP_SNAKES, AEP_SNAKES, AEP_SNAKES, AEP_STEALTH},
            {AEP_THIRST, AEP_MUTAGENIC, AEP_SPEED_DOWN, AEP_NULL},
            {AEA_ADRENALINE, AEA_BUGS, AEA_GROWTH, AEA_NULL},
            {AEA_MUTATE, AEA_SCREAM, AEA_DIM, AEA_NULL}
        },
        {
            _("fractal"),
            _("has a self-similar pattern which repeats until it is too small for you to see"),
            {AEP_ALL_UP, AEP_ALL_UP, AEP_CLAIRVOYANCE, AEP_PSYSHIELD},
            {AEP_SCHIZO, AEP_ATTENTION, AEP_FORCE_TELEPORT, AEP_BAD_WEATHER},
            {AEA_STORM, AEA_FATIGUE, AEA_TELEPORT, AEA_NULL},
            {AEA_RADIATION, AEA_MUTATE, AEA_TELEGLOW, AEA_TELEGLOW}
        }
    };
    for(int i = 0; i < ARTPROP_MAX; i++) {
        artifact_property_data[i] = tmp_artifact_property_data[i];
    }

    artifact_tool_form_datum tmp_artifact_tool_form_data[NUM_ARTTOOLFORMS] = {
        {
            "", '*', c_white, material_id( "null" ), 0_ml, 0_ml, 0, 0, ARTWEAP_BULK,
            {ARTWEAP_NULL, ARTWEAP_NULL, ARTWEAP_NULL}
        },

        {
            _("Harp"), ';', c_yellow, material_id( "wood" ), 5000_ml, 7500_ml, 1150, 2100, ARTWEAP_BULK,
            {ARTWEAP_SPEAR, ARTWEAP_SWORD, ARTWEAP_KNIFE}
        },

        {
            _("Staff"), '/', c_brown, material_id( "wood" ), 1500_ml, 3000_ml, 450, 1150, ARTWEAP_CLUB,
            {ARTWEAP_BULK, ARTWEAP_SPEAR, ARTWEAP_KNIFE}
        },

        {
            _("Sword"), '/', c_ltblue, material_id( "steel" ), 2000_ml, 3500_ml, 900, 3259, ARTWEAP_SWORD,
            {ARTWEAP_BULK, ARTWEAP_NULL, ARTWEAP_NULL}
        },

        {
            _("Dagger"), ';', c_ltblue, material_id( "steel" ), 250_ml, 1000_ml, 100, 700, ARTWEAP_KNIFE,
            {ARTWEAP_NULL, ARTWEAP_NULL, ARTWEAP_NULL}
        },

        {
            _("Cube"), '*', c_white, material_id( "steel" ), 250_ml, 750_ml, 100, 2300, ARTWEAP_BULK,
            {ARTWEAP_SPEAR, ARTWEAP_NULL, ARTWEAP_NULL}
        }
    };
    for(int i = 0; i < NUM_ARTTOOLFORMS; i++) {
        artifact_tool_form_data[i] = tmp_artifact_tool_form_data[i];
    }

    artifact_weapon_datum tmp_artifact_weapon_data[NUM_ARTWEAPS] = {
        { "", 0_ml, 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},
        // Adjective      Vol   Weight Bashing Cutting Stabbing To-hit Flag
        { _( "Heavy" ),   0_ml,   1400, 10, 20,  0,  0,  0,  0, -2, 0, "" },
        { _( "Knobbed" ), 250_ml,  250, 14, 30,  0,  0,  0,  0, -1, 1, "" },
        { _( "Spiked" ),  250_ml,  100,  0,  0,  0,  0, 20, 40, -1, 1, "" },
        { _( "Edged" ),   500_ml,  450,  0,  0, 20, 50,  0,  0, -1, 2, "SHEATH_SWORD" },
        { _( "Bladed" ),  250_ml, 2250,  0,  0,  0,  0, 12, 30, -1, 1, "SHEATH_KNIFE" }
    };
    for(int i = 0; i < NUM_ARTWEAPS; i++) {
        artifact_weapon_data[i] = tmp_artifact_weapon_data[i];
    }

    artifact_armor_form_datum tmp_artifact_armor_form_data[NUM_ARTARMFORMS] = {
        {
            "", c_white, material_id( "null" ),        0_ml,  0,  0,  0,  0,  0,  0,  0_ml,  0,  0,  0,
            0, false,
            {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}
        },
        // Name    color  Material         Vol Wgt Enc Cov Thk Env Wrm Sto Bsh Cut Hit
        {
            _("Robe"),   c_red, material_id( "wool" ),    1500_ml, 700,  1,  90,  3,  0,  2,  0_ml, -8,  0, -3,
            mfb(bp_torso) | mfb(bp_leg_l) | mfb(bp_leg_r), false,
            {
                ARMORMOD_LIGHT, ARMORMOD_BULKY, ARMORMOD_POCKETED, ARMORMOD_FURRED,
                ARMORMOD_PADDED
            }
        },

        {
            _("Coat"),   c_brown, material_id( "leather" ),   3500_ml, 1600,  2,  80, 2,  1,  4,  1000_ml, -6,  0, -3,
            mfb(bp_torso), false,
            {
                ARMORMOD_LIGHT, ARMORMOD_POCKETED, ARMORMOD_FURRED, ARMORMOD_PADDED,
                ARMORMOD_PLATED
            }
        },

        {
            _("Mask"),   c_white, material_id( "wood" ),      1000_ml, 100,  2,  50, 2,  1,  2,  0_ml,  2,  0, -2,
            mfb(bp_eyes) | mfb(bp_mouth), false,
            {
                ARMORMOD_FURRED, ARMORMOD_FURRED, ARMORMOD_NULL, ARMORMOD_NULL,
                ARMORMOD_NULL
            }
        },

        // Name    color  Materials             Vol  Wgt Enc Cov Thk Env Wrm Sto Bsh Cut Hit
        {
            _("Helm"),   c_dkgray, material_id( "silver" ),    1500_ml, 700,  2,  85, 3,  0,  1,  0_ml,  8,  0, -2,
            mfb(bp_head), false,
            {
                ARMORMOD_BULKY, ARMORMOD_FURRED, ARMORMOD_PADDED, ARMORMOD_PLATED,
                ARMORMOD_NULL
            }
        },

        {
            _("Gloves"), c_ltblue, material_id( "leather" ), 500_ml, 100,  1,  90,  3,  1,  2,  0_ml, -4,  0, -2,
            mfb(bp_hand_l) | mfb(bp_hand_r), true,
            {
                ARMORMOD_BULKY, ARMORMOD_FURRED, ARMORMOD_PADDED, ARMORMOD_PLATED,
                ARMORMOD_NULL
            }
        },

        // Name    color  Materials            Vol  Wgt Enc Cov Thk Env Wrm Sto Bsh Cut Hit
        {
            _("Boots"), c_blue, material_id( "leather" ),     1500_ml, 250,  1,  75,  3,  1,  3,  0_ml,  4,  0, -1,
            mfb(bp_foot_l) | mfb(bp_foot_r), true,
            {
                ARMORMOD_LIGHT, ARMORMOD_BULKY, ARMORMOD_PADDED, ARMORMOD_PLATED,
                ARMORMOD_NULL
            }
        },

        {
            _("Ring"), c_ltgreen, material_id( "silver" ),   0_ml,  4,  0,  0,  0,  0,  0,  0_ml,  0,  0,  0,
            0, true,
            {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}
        }

    };
    for(int i = 0; i < NUM_ARTARMFORMS; i++) {
        artifact_armor_form_data[i] = tmp_artifact_armor_form_data[i];
    }

    artifact_armor_form_datum tmp_artifact_armor_mod_data[NUM_ARMORMODS] = {

        {
            "", c_white, material_id( "null" ), 0_ml,  0,  0,  0,  0,  0,  0,  0_ml,  0, 0, 0, 0, false,
            {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}
        },
        // Description; "It is ..." or "They are ..."
        {
            _("very thin and light."), c_white, material_id( "null" ),
            // Vol   Wgt Enc Cov Thk Env Wrm Sto
            -1000_ml, -950, -2, -1, -1, -1, -1,  0_ml, 0, 0, 0, 0,  false,
            {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}
        },

        {
            _("extremely bulky."), c_white, material_id( "null" ),
            2000_ml, 1150,  2,  1,  1,  0,  1,  0_ml, 0, 0, 0, 0,  false,
            {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}
        },

        {
            _("covered in pockets."), c_white, material_id( "null" ),
            250_ml, 150,  1,  0,  0,  0,  0, 4000_ml, 0, 0, 0, 0,  false,
            {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}
        },

        {
            _("disgustingly furry."), c_white, material_id( "wool" ),
            // Vol  Wgt Enc Dmg Cut Env Wrm Sto
            1000_ml, 250,  1,  1,  1,  1,  3,  0_ml, 0, 0, 0, 0,  false,
            {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}
        },

        {
            _("leather-padded."), c_white, material_id( "leather" ),
            1000_ml, 450,  1, 1,  1,  0,  1, -750_ml, 0, 0, 0, 0,  false,
            {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}
        },

        {
            _("plated in iron."), c_white, material_id( "iron" ),
            1000_ml, 1400,  3,  2, 2,  0,  1, -1000_ml, 0, 0, 0, 0, false,
            {ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}
        },

    };
    for(int i = 0; i < NUM_ARMORMODS; i++) {
        artifact_armor_mod_data[i] = tmp_artifact_armor_mod_data[i];
    }

    std::string tmp_artifact_adj[NUM_ART_ADJS] = {
        _("Forbidden"), _("Unknown"), _("Forgotten"), _("Hideous"), _("Eldritch"),
        _("Gelatinous"), _("Ancient"), _("Cursed"), _("Bloody"), _("Undying"),
        _("Shadowy"), _("Silent"), _("Cyclopean"), _("Fungal"), _("Unspeakable"),
        _("Grotesque"), _("Frigid"), _("Shattered"), _("Sleeping"), _("Repellent")
    };
    for(int i = 0; i < NUM_ART_ADJS; i++) {
        artifact_adj[i] = tmp_artifact_adj[i];
    }

    std::string tmp_artifact_noun[NUM_ART_NOUNS] = {
        _("%s Technique"), _("%s Dreams"), _("%s Beasts"), _("%s Evil"), _("%s Miasma"),
        _("the %s Abyss"), _("the %s City"), _("%s Shadows"), _("%s Shade"), _("%s Illusion"),
        _("%s Justice"), _("the %s Necropolis"), _("%s Ichor"), _("the %s Monolith"), _("%s Aeons"),
        _("%s Graves"), _("%s Horrors"), _("%s Suffering"), _("%s Death"), _("%s Horror")
    };
    for(int i = 0; i < NUM_ART_NOUNS; i++) {
        artifact_noun[i] = tmp_artifact_noun[i];
    }

}

void it_artifact_tool::create_name(const std::string &type)
{
    name = artifact_name(type);
    name_plural = name;
}

void it_artifact_tool::create_name(const std::string &property_name, const std::string &shape_name)
{
    name = string_format( pgettext( "artifact name (property, shape)", "%1$s %2$s" ), property_name.c_str(),
                          shape_name.c_str() );
    name_plural = name;
}

void it_artifact_armor::create_name(const std::string &type)
{
    name = artifact_name(type);
    name_plural = name;
}

std::string new_artifact()
{
    if (one_in(2)) { // Generate a "tool" artifact

        it_artifact_tool def;
        auto art = &def; // avoid huge number of line changes

        int form = rng(ARTTOOLFORM_NULL + 1, NUM_ARTTOOLFORMS - 1);

        artifact_tool_form_datum *info = &(artifact_tool_form_data[form]);
        art->create_name(info->name);
        art->color = info->color;
        art->sym = std::string( 1, info->sym );
        art->materials.push_back(info->material);
        art->volume = rng(info->volume_min, info->volume_max);
        art->weight = rng(info->weight_min, info->weight_max);
        // Set up the basic weapon type
        artifact_weapon_datum *weapon = &(artifact_weapon_data[info->base_weapon]);
        art->melee[DT_BASH] = rng(weapon->bash_min, weapon->bash_max);
        art->melee[DT_CUT] = rng(weapon->cut_min, weapon->cut_max);
        art->melee[DT_STAB] = rng(weapon->stab_min, weapon->stab_max);
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
                art->melee[DT_BASH] += rng(weapon->bash_min, weapon->bash_max);
                art->melee[DT_CUT] += rng(weapon->cut_min, weapon->cut_max);
                art->melee[DT_STAB] += rng(weapon->stab_min, weapon->stab_max);
                art->m_to_hit += rng(weapon->to_hit_min, weapon->to_hit_max);
                if( weapon->tag != "" ) {
                    art->item_tags.insert(weapon->tag);
                }
                std::ostringstream newname;
                newname << weapon->adjective << " " << info->name;
                art->create_name(newname.str());
            }
        }
        art->description = string_format(
                               _("This is the %s.\nIt is the only one of its kind.\nIt may have unknown powers; try activating them."),
                               art->nname(1).c_str());

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
                passive_tmp = random_entry_removed( good_effects );
                num_good++;
            } else if (!bad_effects.empty()) { // Bad effect
                passive_tmp = random_entry_removed( bad_effects );
                num_bad++;
            }
            value += passive_effect_cost[passive_tmp];
            art->artifact->effects_wielded.push_back(passive_tmp);
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
                passive_tmp = random_entry_removed( good_effects );
                num_good++;
            } else { // Bad effect
                passive_tmp = random_entry_removed( bad_effects );
                num_bad++;
            }
            value += passive_effect_cost[passive_tmp];
            art->artifact->effects_carried.push_back(passive_tmp);
        }
        // Finally, activated effects; not necessarily good or bad
        num_good = 0;
        num_bad = 0;
        value = 0;
        std::vector<art_effect_active> good_a_effects = fill_good_active();
        std::vector<art_effect_active> bad_a_effects = fill_bad_active();
        while (!good_a_effects.empty() && !bad_a_effects.empty() &&
               num_good < 3 && num_bad < 3 &&
               (value > 3 || (num_bad > 0 && num_good == 0) ||
                !one_in(3 - num_good) || !one_in(3 - num_bad))) {
            if (!one_in(3) && value <= 1) { // Good effect
                active_tmp = random_entry_removed( good_a_effects );
                num_good++;
                value += active_effect_cost[active_tmp];
            } else { // Bad effect
                active_tmp = random_entry_removed( bad_a_effects );
                num_bad++;
                value += active_effect_cost[active_tmp];
            }
            art->artifact->effects_activated.push_back(active_tmp);
            art->tool->max_charges += rng(1, 3);
        }
        art->tool->def_charges = art->tool->max_charges;
        // If we have charges, pick a recharge mechanism
        if( art->tool->max_charges > 0 ) {
            art->artifact->charge_type = art_charge( rng(ARTC_NULL + 1, NUM_ARTCS - 1) );
        }
        if (one_in(8) && num_bad + num_good >= 4) {
            art->artifact->charge_type = ARTC_NULL;    // 1 in 8 chance that it can't recharge!
        }
        item_controller->add_item_type( static_cast<itype &>( *art ) );
        return art->get_id();
    } else { // Generate an armor artifact

        it_artifact_armor def;
        auto art = &def; // avoid huge number of line changes

        int form = rng(ARTARMFORM_NULL + 1, NUM_ARTARMFORMS - 1);
        artifact_armor_form_datum *info = &(artifact_armor_form_data[form]);

        art->create_name(info->name);
        art->sym = "["; // Armor is always [
        art->color = info->color;
        art->materials.push_back(info->material);
        art->volume = info->volume;
        art->weight = info->weight;
        art->melee[DT_BASH] = info->melee_bash;
        art->melee[DT_CUT] = info->melee_cut;
        art->m_to_hit = info->melee_hit;
        art->armor->covers = info->covers;
        art->armor->encumber = info->encumb;
        art->armor->coverage = info->coverage;
        art->armor->thickness = info->thickness;
        art->armor->env_resist = info->env_resist;
        art->armor->warmth = info->warmth;
        art->armor->storage = info->storage;
        std::ostringstream description;
        description << string_format(info->plural ?
                                     _("This is the %s.\nThey are the only ones of their kind.") :
                                     _("This is the %s.\nIt is the only one of its kind."),
                                     art->nname(1).c_str());

        // Modify the armor further
        if (!one_in(4)) {
            int index = rng(0, 4);
            if (info->available_mods[index] != ARMORMOD_NULL) {
                artifact_armor_mod mod = info->available_mods[index];
                artifact_armor_form_datum *modinfo = &(artifact_armor_mod_data[mod]);
                if( modinfo->volume >= 0 || art->volume > -modinfo->volume ) {
                    art->volume += modinfo->volume;
                } else {
                    art->volume = 250_ml;
                }

                if( modinfo->weight >= 0 || art->weight > std::abs( modinfo->weight ) ) {
                    art->weight += modinfo->weight;
                } else {
                    art->weight = 1;
                }

                art->armor->encumber += modinfo->encumb;

                if( modinfo->coverage > 0 || art->armor->coverage > std::abs( modinfo->coverage ) ) {
                    art->armor->coverage += modinfo->coverage;
                } else {
                    art->armor->coverage = 0;
                }

                if( modinfo->thickness > 0 || art->armor->thickness > std::abs( modinfo->thickness ) ) {
                    art->armor->thickness += modinfo->thickness;
                } else {
                    art->armor->thickness = 0;
                }

                if( modinfo->env_resist > 0 || art->armor->env_resist > std::abs( modinfo->env_resist ) ) {
                    art->armor->env_resist += modinfo->env_resist;
                } else {
                    art->armor->env_resist = 0;
                }
                art->armor->warmth += modinfo->warmth;

                if( modinfo->storage > 0 || art->armor->storage > -modinfo->storage ) {
                    art->armor->storage += modinfo->storage;
                } else {
                    art->armor->storage = 0;
                }

                description << string_format(info->plural ?
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
                passive_tmp = random_entry_removed( good_effects );
                num_good++;
            } else { // Bad effect
                passive_tmp = random_entry_removed( bad_effects );
                num_bad++;
            }
            value += passive_effect_cost[passive_tmp];
            art->artifact->effects_worn.push_back(passive_tmp);
        }
        item_controller->add_item_type( static_cast<itype &>( *art ) );
        return art->get_id();
    }
}

std::string new_natural_artifact(artifact_natural_property prop)
{
    // Natural artifacts are always tools.
    it_artifact_tool def;
    auto art = &def; // avoid huge number of line changes

    // Pick a form
    artifact_natural_shape shape =
        artifact_natural_shape(rng(ARTSHAPE_NULL + 1, ARTSHAPE_MAX - 1));
    artifact_shape_datum *shape_data = &(artifact_shape_data[shape]);
    // Pick a property
    artifact_natural_property property = (prop > ARTPROP_NULL ? prop :
                                          artifact_natural_property(rng(ARTPROP_NULL + 1,
                                                  ARTPROP_MAX - 1)));
    artifact_property_datum *property_data = &(artifact_property_data[property]);

    art->sym = ":";
    art->color = c_yellow;
    art->materials.push_back( material_id( "stone" ) );
    art->volume = rng(shape_data->volume_min, shape_data->volume_max);
    art->weight = rng(shape_data->weight_min, shape_data->weight_max);
    art->melee[DT_BASH] = 0;
    art->melee[DT_CUT] = 0;
    art->m_to_hit = 0;

    art->create_name(property_data->name, shape_data->name);
    art->description = string_format( pgettext( "artifact description", "This %1$s %2$s." ),
                                      shape_data->desc.c_str(), property_data->desc.c_str() );

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
            if (aep_good == AEP_NULL || one_in(4)) {
                aep_good = art_effect_passive(rng(AEP_NULL + 1, AEP_SPLIT - 1));
            }
        }
        if (bad_passive) {
            aep_bad = property_data->passive_bad[ rng(0, 3) ];
            if (aep_bad == AEP_NULL || one_in(4)) {
                aep_bad = art_effect_passive(rng(AEP_SPLIT + 1, NUM_AEAS - 1));
            }
        }
        if (good_active) {
            aea_good = property_data->active_good[ rng(0, 3) ];
            if (aea_good == AEA_NULL || one_in(4)) {
                aea_good = art_effect_active(rng(AEA_NULL + 1, AEA_SPLIT - 1));
            }
        }
        if (bad_active) {
            aea_bad = property_data->active_bad[ rng(0, 3) ];
            if (aea_bad == AEA_NULL || one_in(4)) {
                aea_bad = art_effect_active(rng(AEA_SPLIT + 1, NUM_AEAS - 1));
            }
        }

        value = passive_effect_cost[aep_good] + passive_effect_cost[aep_bad] +
                active_effect_cost[aea_good] +  active_effect_cost[aea_bad];
        value_to_reach++; // Yes, it is intentional that this is 1 the first check
    } while (value > value_to_reach);

    if (aep_good != AEP_NULL) {
        art->artifact->effects_carried.push_back(aep_good);
    }
    if (aep_bad != AEP_NULL) {
        art->artifact->effects_carried.push_back(aep_bad);
    }
    if (aea_good != AEA_NULL) {
        art->artifact->effects_activated.push_back(aea_good);
    }
    if (aea_bad != AEA_NULL) {
        art->artifact->effects_activated.push_back(aea_bad);
    }

    // Natural artifacts ALWAYS can recharge
    // (When "implanting" them in a mundane item, this ability may be lost
    if (!art->artifact->effects_activated.empty()) {
        art->tool->def_charges = art->tool->max_charges = rng( 1, 4 );
        art->artifact->charge_type = art_charge( rng(ARTC_NULL + 1, NUM_ARTCS - 1) );
    }
    item_controller->add_item_type( static_cast<itype &>( *art ) );
    return art->get_id();
}

// Make a special debugging artifact.
std::string architects_cube()
{
    std::string artifact_name(std::string type);

    it_artifact_tool def;
    auto art = &def;

    artifact_tool_form_datum *info = &(artifact_tool_form_data[ARTTOOLFORM_CUBE]);
    art->create_name(info->name);
    art->color = info->color;
    art->sym = std::string( 1, info->sym );
      art->materials.push_back(info->material);
    art->volume = rng(info->volume_min, info->volume_max);
    art->weight = rng(info->weight_min, info->weight_max);
    // Set up the basic weapon type
    artifact_weapon_datum *weapon = &(artifact_weapon_data[info->base_weapon]);
    art->melee[DT_BASH] = rng(weapon->bash_min, weapon->bash_max);
    art->melee[DT_CUT] = rng(weapon->cut_min, weapon->cut_max);
    art->m_to_hit = rng(weapon->to_hit_min, weapon->to_hit_max);
    if( weapon->tag != "" ) {
        art->item_tags.insert(weapon->tag);
    }
    // Add an extra weapon perhaps?
    art->description = _("The architect's cube.");
    art->artifact->effects_carried.push_back(AEP_SUPER_CLAIRVOYANCE);
    item_controller->add_item_type( static_cast<itype &>( def ) );
    return art->get_id();
}

std::vector<art_effect_passive> fill_good_passive()
{
    std::vector<art_effect_passive> ret;
    for (int i = AEP_NULL + 1; i < AEP_SPLIT; i++) {
        ret.push_back( art_effect_passive(i) );
    }
    return ret;
}

std::vector<art_effect_passive> fill_bad_passive()
{
    std::vector<art_effect_passive> ret;
    for (int i = AEP_SPLIT + 1; i < NUM_AEPS; i++) {
        ret.push_back( art_effect_passive(i) );
    }
    return ret;
}

std::vector<art_effect_active> fill_good_active()
{
    std::vector<art_effect_active> ret;
    for (int i = AEA_NULL + 1; i < AEA_SPLIT; i++) {
        ret.push_back( art_effect_active(i) );
    }
    return ret;
}

std::vector<art_effect_active> fill_bad_active()
{
    std::vector<art_effect_active> ret;
    for (int i = AEA_SPLIT + 1; i < NUM_AEAS; i++) {
        ret.push_back( art_effect_active(i) );
    }
    return ret;
}

std::string artifact_name(std::string type)
{
    std::string ret;
    std::string noun = artifact_noun[rng(0, NUM_ART_NOUNS - 1)];
    std::string adj = artifact_adj[rng(0, NUM_ART_ADJS - 1)];
    ret = string_format(noun, adj.c_str());
    ret = string_format( pgettext( "artifact name (type, noun)", "%1$s of %2$s" ), type.c_str(), ret.c_str() );
    return ret;
}


/* Json Loading and saving */

void load_artifacts(const std::string &artfilename)
{
    read_from_file_optional_json( artfilename, []( JsonIn &artifact_json ) {
        artifact_json.start_array();
        while (!artifact_json.end_array()) {
            JsonObject jo = artifact_json.get_object();
            std::string type = jo.get_string("type");
            if (type == "artifact_tool") {
                item_controller->add_item_type( static_cast<const itype &>( it_artifact_tool( jo ) ) );
            } else if (type == "artifact_armor") {
                item_controller->add_item_type( static_cast<const itype &>( it_artifact_armor( jo ) ) );
            } else {
                jo.throw_error( "unrecognized artifact type.", "type" );
            }
        }
    } );
}

void it_artifact_tool::deserialize(JsonObject &jo)
{
    id = jo.get_string("id");
    name = jo.get_string("name");
    description = jo.get_string("description");
    if( jo.has_int( "sym" ) ) {
        sym = std::string( 1, jo.get_int( "sym" ) );
    } else {
        sym = jo.get_string( "sym" );
    }
    color = jo.get_int("color");
    price = jo.get_int("price");
    // LEGACY: Since it seems artifacts get serialized out to disk, and they're
    // dynamic, we need to allow for them to be read from disk for, oh, I guess
    // quite some time. Loading and saving once will write things out as a JSON
    // array.
    if (jo.has_string("m1")) {
        materials.push_back( material_id( jo.get_string( "m1" ) ) );
    }
    if (jo.has_string("m2")) {
        materials.push_back( material_id( jo.get_string( "m2" ) ) );
    }
    // Assumption, perhaps dangerous, that we won't wind up with m1 and m2 and
    // a materials array in our serialized objects at the same time.
    if (jo.has_array("materials")) {
        JsonArray jarr = jo.get_array("materials");
        for( size_t i = 0; i < jarr.size(); ++i) {
            materials.push_back( material_id ( jarr.get_string( i ) ) );
        }
    }
    volume = jo.get_int("volume") * units::legacy_volume_factor;
    weight = jo.get_int("weight");
    melee[DT_BASH] = jo.get_int("melee_dam");
    melee[DT_CUT] = jo.get_int("melee_cut");
    m_to_hit = jo.get_int("m_to_hit");
    item_tags = jo.get_tags("item_flags");

    tool->max_charges = jo.get_long("max_charges");
    tool->def_charges = jo.get_long("def_charges");

    tool->charges_per_use = jo.get_int("charges_per_use");
    tool->turns_per_charge = jo.get_int("turns_per_charge");
    tool->ammo_id = ammotype( jo.get_string("ammo") );
    tool->revert_to = jo.get_string("revert_to");

    artifact->charge_type = (art_charge)jo.get_int("charge_type");

    JsonArray ja = jo.get_array("effects_wielded");
    while (ja.has_more()) {
        artifact->effects_wielded.push_back((art_effect_passive)ja.next_int());
    }

    ja = jo.get_array("effects_activated");
    while (ja.has_more()) {
        artifact->effects_activated.push_back((art_effect_active)ja.next_int());
    }

    ja = jo.get_array("effects_carried");
    while (ja.has_more()) {
        artifact->effects_carried.push_back((art_effect_passive)ja.next_int());
    }
}

void it_artifact_armor::deserialize(JsonObject &jo)
{
    id = jo.get_string("id");
    name = jo.get_string("name");
    description = jo.get_string("description");
    if( jo.has_int( "sym" ) ) {
        sym = std::string( 1, jo.get_int( "sym" ) );
    } else {
        sym = jo.get_string( "sym" );
    }
    color = jo.get_int("color");
    price = jo.get_int("price");
    // LEGACY: Since it seems artifacts get serialized out to disk, and they're
    // dynamic, we need to allow for them to be read from disk for, oh, I guess
    // quite some time. Loading and saving once will write things out as a JSON
    // array.
    if (jo.has_string("m1")) {
        materials.push_back( material_id( jo.get_string( "m1" ) ) );
    }
    if (jo.has_string("m2")) {
        materials.push_back( material_id( jo.get_string( "m2" ) ) );
    }
    // Assumption, perhaps dangerous, that we won't wind up with m1 and m2 and
    // a materials array in our serialized objects at the same time.
    if (jo.has_array("materials")) {
        JsonArray jarr = jo.get_array("materials");
        for( size_t i = 0; i < jarr.size(); ++i) {
            materials.push_back( material_id( jarr.get_string( i ) ) );
        }
    }
    volume = jo.get_int("volume") * units::legacy_volume_factor;
    weight = jo.get_int("weight");
    melee[DT_BASH] = jo.get_int("melee_dam");
    melee[DT_CUT] = jo.get_int("melee_cut");
    m_to_hit = jo.get_int("m_to_hit");
    item_tags = jo.get_tags("item_flags");

    jo.read( "covers", armor->covers);
    armor->encumber = jo.get_int("encumber");
    armor->coverage = jo.get_int("coverage");
    armor->thickness = jo.get_int("material_thickness");
    armor->env_resist = jo.get_int("env_resist");
    armor->warmth = jo.get_int("warmth");
    armor->storage = jo.get_int("storage") * units::legacy_volume_factor;
    armor->power_armor = jo.get_bool("power_armor");

    JsonArray ja = jo.get_array("effects_worn");
    while (ja.has_more()) {
        artifact->effects_worn.push_back((art_effect_passive)ja.next_int());
    }
}

bool save_artifacts( const std::string &path )
{
    return write_to_file_exclusive( path, [&]( std::ostream &fout ) {
        JsonOut json( fout );
        json.start_array();
        for( const itype *e : item_controller->all() ) {
            if( !e->artifact ) {
                continue;
            }

            if( e->tool ) {
                json.write( it_artifact_tool( *e ) );

            } else if( e->armor ) {
                json.write( it_artifact_armor( *e ) );
            }
        }
        json.end_array();
    }, _( "artifact file" ) );
}

template<typename E>
void serialize_enum_vector_as_int( JsonOut &json, const std::string &member, const std::vector<E> &vec )
{
    json.member( member );
    json.start_array();
    for( auto & e : vec ) {
        json.write( static_cast<int>( e ) );
    }
    json.end_array();
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
    json.member("color", color);
    json.member("price", price);
    json.member("materials");
    json.start_array();
    for (auto mat : materials) {
        json.write(mat);
    }
    json.end_array();
    json.member("volume", volume / units::legacy_volume_factor);
    json.member("weight", weight);

    json.member( "melee_dam", melee[DT_BASH] );
    json.member( "melee_cut", melee[DT_CUT] );

    json.member("m_to_hit", m_to_hit);

    json.member("item_flags", item_tags);
    json.member("techniques", techniques);

    // tool data
    json.member("ammo", tool->ammo_id);
    json.member("max_charges", tool->max_charges);
    json.member("def_charges", tool->def_charges);
    json.member("charges_per_use", tool->charges_per_use);
    json.member("turns_per_charge", tool->turns_per_charge);
    json.member("revert_to", tool->revert_to);

    // artifact data
    json.member("charge_type", artifact->charge_type);
    serialize_enum_vector_as_int( json, "effects_wielded", artifact->effects_wielded );
    serialize_enum_vector_as_int( json, "effects_activated", artifact->effects_activated );
    serialize_enum_vector_as_int( json, "effects_carried", artifact->effects_carried );

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
    json.member("color", color);
    json.member("price", price);
    json.member("materials");
    json.start_array();
    for (auto mat : materials) {
        json.write(mat);
    }
    json.end_array();
    json.member("volume", volume / units::legacy_volume_factor);
    json.member("weight", weight);

    json.member( "melee_dam", melee[DT_BASH] );
    json.member( "melee_cut", melee[DT_CUT] );

    json.member("m_to_hit", m_to_hit);

    json.member("item_flags", item_tags);

    json.member("techniques", techniques);

    // armor data
    json.member("covers", armor->covers);
    json.member("encumber", armor->encumber);
    json.member("coverage", armor->coverage);
    json.member("material_thickness", armor->thickness);
    json.member("env_resist", armor->env_resist);
    json.member("warmth", armor->warmth);
    json.member("storage", armor->storage / units::legacy_volume_factor);
    json.member("power_armor", armor->power_armor);

    // artifact data
    serialize_enum_vector_as_int( json, "effects_worn", artifact->effects_worn );

    json.end_object();
}

namespace io {
#define PAIR(x) { #x, x }
static const std::unordered_map<std::string, art_effect_passive> art_effect_passive_values = { {
    //PAIR( AEP_NULL ), // not really used
    PAIR( AEP_STR_UP ),
    PAIR( AEP_DEX_UP ),
    PAIR( AEP_PER_UP ),
    PAIR( AEP_INT_UP ),
    PAIR( AEP_ALL_UP ),
    PAIR( AEP_SPEED_UP ),
    PAIR( AEP_PBLUE ),
    PAIR( AEP_SNAKES ),
    PAIR( AEP_INVISIBLE ),
    PAIR( AEP_CLAIRVOYANCE ),
    PAIR( AEP_SUPER_CLAIRVOYANCE ),
    PAIR( AEP_STEALTH ),
    PAIR( AEP_EXTINGUISH ),
    PAIR( AEP_GLOW ),
    PAIR( AEP_PSYSHIELD ),
    PAIR( AEP_RESIST_ELECTRICITY ),
    PAIR( AEP_CARRY_MORE ),
    PAIR( AEP_SAP_LIFE ),
    //PAIR( AEP_SPLIT, // not really used
    PAIR( AEP_HUNGER ),
    PAIR( AEP_THIRST ),
    PAIR( AEP_SMOKE ),
    PAIR( AEP_EVIL ),
    PAIR( AEP_SCHIZO ),
    PAIR( AEP_RADIOACTIVE ),
    PAIR( AEP_MUTAGENIC ),
    PAIR( AEP_ATTENTION ),
    PAIR( AEP_STR_DOWN ),
    PAIR( AEP_DEX_DOWN ),
    PAIR( AEP_PER_DOWN ),
    PAIR( AEP_INT_DOWN ),
    PAIR( AEP_ALL_DOWN ),
    PAIR( AEP_SPEED_DOWN ),
    PAIR( AEP_FORCE_TELEPORT ),
    PAIR( AEP_MOVEMENT_NOISE ),
    PAIR( AEP_BAD_WEATHER ),
    PAIR( AEP_SICK ),
} };
static const std::unordered_map<std::string, art_effect_active> art_effect_active_values = { {
    //PAIR( AEA_NULL ), // not really used
    PAIR( AEA_STORM ),
    PAIR( AEA_FIREBALL ),
    PAIR( AEA_ADRENALINE ),
    PAIR( AEA_MAP ),
    PAIR( AEA_BLOOD ),
    PAIR( AEA_FATIGUE ),
    PAIR( AEA_ACIDBALL ),
    PAIR( AEA_PULSE ),
    PAIR( AEA_HEAL ),
    PAIR( AEA_CONFUSED ),
    PAIR( AEA_ENTRANCE ),
    PAIR( AEA_BUGS ),
    PAIR( AEA_TELEPORT ),
    PAIR( AEA_LIGHT ),
    PAIR( AEA_GROWTH ),
    PAIR( AEA_HURTALL ),
    //PAIR( AEA_SPLIT ), // not really used
    PAIR( AEA_RADIATION ),
    PAIR( AEA_PAIN ),
    PAIR( AEA_MUTATE ),
    PAIR( AEA_PARALYZE ),
    PAIR( AEA_FIRESTORM ),
    PAIR( AEA_ATTENTION ),
    PAIR( AEA_TELEGLOW ),
    PAIR( AEA_NOISE ),
    PAIR( AEA_SCREAM ),
    PAIR( AEA_DIM ),
    PAIR( AEA_FLASH ),
    PAIR( AEA_VOMIT ),
    PAIR( AEA_SHADOWS ),
} };
static const std::unordered_map<std::string, art_charge> art_charge_values = { {
    PAIR( ARTC_NULL ),
    PAIR( ARTC_TIME ),
    PAIR( ARTC_SOLAR ),
    PAIR( ARTC_PAIN ),
    PAIR( ARTC_HP ),
} };
#undef PAIR

template<>
art_effect_passive string_to_enum<art_effect_passive>( const std::string &data )
{
    return string_to_enum_look_up( art_effect_passive_values, data );
}

template<>
art_effect_active string_to_enum<art_effect_active>( const std::string &data )
{
    return string_to_enum_look_up( art_effect_active_values, data );
}

template<>
art_charge string_to_enum<art_charge>( const std::string &data )
{
    return string_to_enum_look_up( art_charge_values, data );
}
} // namespace io
