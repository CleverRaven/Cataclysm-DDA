#include "artifact.h"

#include <cstdlib>
#include <array>
#include <sstream>
#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include "assign.h"
#include "cata_utility.h"
#include "item_factory.h"
#include "json.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"
#include "bodypart.h"
#include "color.h"
#include "damage.h"
#include "iuse.h"
#include "optional.h"
#include "units.h"
#include "type_id.h"

template<typename V, typename B>
inline units::quantity<V, B> rng( const units::quantity<V, B> &min,
                                  const units::quantity<V, B> &max )
{
    return units::quantity<V, B>( rng( min.value(), max.value() ), B{} );
}

std::vector<art_effect_passive> fill_good_passive();
std::vector<art_effect_passive> fill_bad_passive();
std::vector<art_effect_active>  fill_good_active();
std::vector<art_effect_active>  fill_bad_active();

static const std::array<int, NUM_AEPS> passive_effect_cost = { {
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
        7, // AEP_CLAIRVOYANCE_PLUS
        50,// AEP_SUPER_CLAIRVOYANCE
        2, // AEP_STEALTH
        2, // AEP_EXTINGUISH
        1, // AEP_GLOW
        1, // AEP_PSYSHIELD
        3, // AEP_RESIST_ELECTRICITY
        3, // AEP_CARRY_MORE
        5, // AEP_SAP_LIFE
        1, // AEP_FUN

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
    }
};

static const std::array<int, NUM_AEAS> active_effect_cost = { {
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
        2, // AEA_FUN

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
        - 2 // AEA_STAMINA_EMPTY
    }
};

enum artifact_natural_shape {
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
    units::mass weight_min, weight_max;
};

struct artifact_property_datum {
    std::string name;
    std::string desc;
    std::array<art_effect_passive, 4> passive_good;
    std::array<art_effect_passive, 4> passive_bad;
    std::array<art_effect_active, 4> active_good;
    std::array<art_effect_active, 4> active_bad;
};

struct artifact_dream_datum {     //Used only when generating - stored as individual members of each artifact
    std::vector<std::string> msg_unmet;
    std::vector<std::string> msg_met;
    // Once per hour while sleeping, makes a list of each artifact that passes a (freq) in 100 check
    // One item is picked from artifacts that passed that chance, and the appropriate msg is shown
    // If multiple met/unmet messages are specified for the item, one is picked at random
    int freq_unmet; // 100 if no reqs, since should never be unmet in that case
    int freq_met;   //   0 if no reqs
};

enum artifact_weapon_type {
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
    deferred_color color;
    // Most things had 0 to 1 material.
    material_id material;
    units::volume volume_min, volume_max;
    units::mass weight_min, weight_max;
    artifact_weapon_type base_weapon;
    std::array<artifact_weapon_type, 3> extra_weapons;
};

enum artifact_tool_form {
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
    units::mass weight; // Only applicable if this is an *extra* weapon
    int bash_min;
    int bash_max;
    int cut_min;
    int cut_max;
    int stab_min;
    int stab_max;
    int to_hit_min;
    int to_hit_max;
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
    deferred_color color;
    // Most things had 0 to 1 material.
    material_id material;
    units::volume volume;
    units::mass weight;
    int encumb;
    int max_encumb;
    int coverage;
    int thickness;
    int env_resist;
    int warmth;
    units::volume storage;
    int melee_bash;
    int melee_cut;
    int melee_hit;
    body_part_set covers;
    bool plural;
    std::array<artifact_armor_mod, 5> available_mods;
};

enum artifact_armor_form {
    ARTARMFORM_ROBE,
    ARTARMFORM_COAT,
    ARTARMFORM_MASK,
    ARTARMFORM_HELM,
    ARTARMFORM_GLOVES,
    ARTARMFORM_BOOTS,
    ARTARMFORM_RING,
    NUM_ARTARMFORMS
};

static const std::array<artifact_shape_datum, ARTSHAPE_MAX> artifact_shape_data = { {
        {translate_marker( "sphere" ), translate_marker( "smooth sphere" ), 500_ml, 1000_ml, 1_gram, 1150_gram},
        {translate_marker( "rod" ), translate_marker( "tapered rod" ), 250_ml, 1750_ml, 1_gram, 800_gram},
        {translate_marker( "teardrop" ), translate_marker( "teardrop-shaped stone" ), 500_ml, 1500_ml, 1_gram, 950_gram},
        {translate_marker( "lamp" ), translate_marker( "hollow, transparent cube" ), 1000_ml, 225_ml, 1_gram, 350_gram},
        {translate_marker( "snake" ), translate_marker( "winding, flexible rod" ), 0_ml, 2000_ml, 1_gram, 950_gram},
        {translate_marker( "disc" ), translate_marker( "smooth disc" ), 1000_ml, 1500_ml, 200_gram, 400_gram},
        {translate_marker( "beads" ), translate_marker( "string of beads" ), 750_ml, 1750_ml, 1_gram, 700_gram},
        {translate_marker( "napkin" ), translate_marker( "very thin sheet" ), 0_ml, 750_ml, 1_gram, 350_gram},
        {translate_marker( "urchin" ), translate_marker( "spiked sphere" ), 750_ml, 1250_ml, 200_gram, 700_gram},
        {translate_marker( "jelly" ), translate_marker( "malleable blob" ), 500_ml, 2000_ml, 200_gram, 450_gram},
        {translate_marker( "spiral" ), translate_marker( "spiraling rod" ), 1250_ml, 1500_ml, 200_gram, 350_gram},
        {translate_marker( "pin" ), translate_marker( "pointed rod" ), 250_ml, 1250_ml, 100_gram, 1050_gram},
        {translate_marker( "tube" ), translate_marker( "hollow tube" ), 500_ml, 1250_ml, 350_gram, 700_gram},
        {translate_marker( "pyramid" ), translate_marker( "regular tetrahedron" ), 750_ml, 1750_ml, 200_gram, 450_gram},
        {translate_marker( "crystal" ), translate_marker( "translucent crystal" ), 250_ml, 1500_ml, 200_gram, 800_gram},
        {translate_marker( "knot" ), translate_marker( "twisted, knotted cord" ), 500_ml, 1500_ml, 100_gram, 800_gram},
        {translate_marker( "crescent" ), translate_marker( "crescent-shaped stone" ), 500_ml, 1500_ml, 200_gram, 700_gram}
    }
};
static const std::array<artifact_property_datum, ARTPROP_MAX> artifact_property_data = { {
        {
            "BUG", "BUG",
            {{AEP_NULL, AEP_NULL, AEP_NULL, AEP_NULL}},
            {{AEP_NULL, AEP_NULL, AEP_NULL, AEP_NULL}},
            {{AEA_NULL, AEA_NULL, AEA_NULL, AEA_NULL}},
            {{AEA_NULL, AEA_NULL, AEA_NULL, AEA_NULL}}
        },
        {
            translate_marker( "wriggling" ), translate_marker( "is constantly wriggling" ),
            {{AEP_SPEED_UP, AEP_SNAKES, AEP_FUN, AEP_NULL}},
            {{AEP_DEX_DOWN, AEP_FORCE_TELEPORT, AEP_SICK, AEP_NULL}},
            {{AEA_TELEPORT, AEA_ADRENALINE, AEA_FUN, AEA_NULL}},
            {{AEA_MUTATE, AEA_ATTENTION, AEA_VOMIT, AEA_NULL}}
        },
        {
            translate_marker( "glowing" ), translate_marker( "glows faintly" ),
            {{AEP_INT_UP, AEP_GLOW, AEP_CLAIRVOYANCE, AEP_NULL}},
            {{AEP_RADIOACTIVE, AEP_MUTAGENIC, AEP_ATTENTION, AEP_NULL}},
            {{AEA_LIGHT, AEA_LIGHT, AEA_LIGHT, AEA_NULL}},
            {{AEA_ATTENTION, AEA_TELEGLOW, AEA_FLASH, AEA_SHADOWS}}
        },
        {
            translate_marker( "humming" ), translate_marker( "hums very quietly" ),
            {{AEP_ALL_UP, AEP_PSYSHIELD, AEP_FUN, AEP_NULL}},
            {{AEP_SCHIZO, AEP_PER_DOWN, AEP_INT_DOWN, AEP_NULL}},
            {{AEA_PULSE, AEA_ENTRANCE, AEA_FUN, AEA_NULL}},
            {{AEA_NOISE, AEA_NOISE, AEA_SCREAM, AEA_STAMINA_EMPTY}}
        },
        {
            translate_marker( "moving" ), translate_marker( "shifts from side to side slowly" ),
            {{AEP_STR_UP, AEP_DEX_UP, AEP_SPEED_UP, AEP_NULL}},
            {{AEP_HUNGER, AEP_PER_DOWN, AEP_FORCE_TELEPORT, AEP_NULL}},
            {{AEA_TELEPORT, AEA_TELEPORT, AEA_MAP, AEA_NULL}},
            {{AEA_PARALYZE, AEA_VOMIT, AEA_VOMIT, AEA_NULL}}
        },
        {
            translate_marker( "whispering" ), translate_marker( "makes very faint whispering sounds" ),
            {{AEP_CLAIRVOYANCE, AEP_EXTINGUISH, AEP_STEALTH, AEP_NULL}},
            {{AEP_EVIL, AEP_SCHIZO, AEP_ATTENTION, AEP_NULL}},
            {{AEA_FATIGUE, AEA_ENTRANCE, AEA_ENTRANCE, AEA_NULL}},
            {{AEA_ATTENTION, AEA_SCREAM, AEA_SCREAM, AEA_SHADOWS}}
        },
        {
            translate_marker( "breathing" ),
            translate_marker( "shrinks and grows very slightly with a regular pulse, as if breathing" ),
            {{AEP_SAP_LIFE, AEP_ALL_UP, AEP_SPEED_UP, AEP_CARRY_MORE}},
            {{AEP_HUNGER, AEP_THIRST, AEP_SICK, AEP_BAD_WEATHER}},
            {{AEA_ADRENALINE, AEA_HEAL, AEA_ENTRANCE, AEA_GROWTH}},
            {{AEA_MUTATE, AEA_ATTENTION, AEA_SHADOWS, AEA_STAMINA_EMPTY}}
        },
        {
            translate_marker( "dead" ), translate_marker( "is icy cold to the touch" ),
            {{AEP_INVISIBLE, AEP_CLAIRVOYANCE, AEP_EXTINGUISH, AEP_SAP_LIFE}},
            {{AEP_HUNGER, AEP_EVIL, AEP_ALL_DOWN, AEP_SICK}},
            {{AEA_BLOOD, AEA_HURTALL, AEA_NULL, AEA_NULL}},
            {{AEA_PAIN, AEA_SHADOWS, AEA_DIM, AEA_VOMIT}}
        },
        {
            translate_marker( "itchy" ), translate_marker( "makes your skin itch slightly when it is close" ),
            {{AEP_DEX_UP, AEP_SPEED_UP, AEP_PSYSHIELD, AEP_FUN}},
            {{AEP_RADIOACTIVE, AEP_MUTAGENIC, AEP_SICK, AEP_NULL}},
            {{AEA_ADRENALINE, AEA_BLOOD, AEA_HEAL, AEA_BUGS}},
            {{AEA_RADIATION, AEA_PAIN, AEA_PAIN, AEA_VOMIT}}
        },
        {
            translate_marker( "glittering" ), translate_marker( "glitters faintly under direct light" ),
            {{AEP_INT_UP, AEP_EXTINGUISH, AEP_GLOW, AEP_NULL}},
            {{AEP_SMOKE, AEP_ATTENTION, AEP_NULL, AEP_NULL}},
            {{AEA_MAP, AEA_LIGHT, AEA_CONFUSED, AEA_ENTRANCE}},
            {{AEA_RADIATION, AEA_MUTATE, AEA_ATTENTION, AEA_FLASH}}
        },
        {
            translate_marker( "electric" ), translate_marker( "very weakly shocks you when touched" ),
            {{AEP_RESIST_ELECTRICITY, AEP_DEX_UP, AEP_SPEED_UP, AEP_PSYSHIELD}},
            {{AEP_THIRST, AEP_SMOKE, AEP_STR_DOWN, AEP_BAD_WEATHER}},
            {{AEA_STORM, AEA_ADRENALINE, AEA_LIGHT, AEA_FUN}},
            {{AEA_PAIN, AEA_PARALYZE, AEA_FLASH, AEA_FLASH}}
        },
        {
            translate_marker( "slimy" ), translate_marker( "feels slimy" ),
            {{AEP_SNAKES, AEP_STEALTH, AEP_EXTINGUISH, AEP_SAP_LIFE}},
            {{AEP_THIRST, AEP_DEX_DOWN, AEP_SPEED_DOWN, AEP_SICK}},
            {{AEA_BLOOD, AEA_ACIDBALL, AEA_GROWTH, AEA_ACIDBALL}},
            {{AEA_MUTATE, AEA_MUTATE, AEA_VOMIT, AEA_VOMIT}}
        },
        {
            translate_marker( "engraved" ), translate_marker( "is covered with odd etchings" ),
            {{AEP_CLAIRVOYANCE, AEP_INVISIBLE, AEP_PSYSHIELD, AEP_SAP_LIFE}},
            {{AEP_EVIL, AEP_ATTENTION, AEP_NULL, AEP_NULL}},
            {{AEA_FATIGUE, AEA_TELEPORT, AEA_HEAL, AEA_FATIGUE}},
            {{AEA_ATTENTION, AEA_ATTENTION, AEA_TELEGLOW, AEA_DIM}}
        },
        {
            translate_marker( "crackling" ), translate_marker( "occasionally makes a soft crackling sound" ),
            {{AEP_EXTINGUISH, AEP_RESIST_ELECTRICITY, AEP_NULL, AEP_NULL}},
            {{AEP_SMOKE, AEP_RADIOACTIVE, AEP_MOVEMENT_NOISE, AEP_NULL}},
            {{AEA_STORM, AEA_FIREBALL, AEA_PULSE, AEA_NULL}},
            {{AEA_PAIN, AEA_PARALYZE, AEA_NOISE, AEA_NOISE}}
        },
        {
            translate_marker( "warm" ), translate_marker( "is warm to the touch" ),
            {{AEP_STR_UP, AEP_EXTINGUISH, AEP_GLOW, AEP_FUN}},
            {{AEP_SMOKE, AEP_RADIOACTIVE, AEP_NULL, AEP_NULL}},
            {{AEA_FIREBALL, AEA_FIREBALL, AEA_FIREBALL, AEA_LIGHT}},
            {{AEA_FIRESTORM, AEA_FIRESTORM, AEA_TELEGLOW, AEA_NULL}}
        },
        {
            translate_marker( "rattling" ), translate_marker( "makes a rattling sound when moved" ),
            {{AEP_DEX_UP, AEP_SPEED_UP, AEP_SNAKES, AEP_CARRY_MORE}},
            {{AEP_ATTENTION, AEP_INT_DOWN, AEP_MOVEMENT_NOISE, AEP_MOVEMENT_NOISE}},
            {{AEA_BLOOD, AEA_PULSE, AEA_BUGS, AEA_NULL}},
            {{AEA_PAIN, AEA_ATTENTION, AEA_NOISE, AEA_NULL}}
        },
        {
            translate_marker( "scaled" ), translate_marker( "has a surface reminiscent of reptile scales" ),
            {{AEP_SNAKES, AEP_SNAKES, AEP_SNAKES, AEP_STEALTH}},
            {{AEP_THIRST, AEP_MUTAGENIC, AEP_SPEED_DOWN, AEP_NULL}},
            {{AEA_ADRENALINE, AEA_BUGS, AEA_GROWTH, AEA_NULL}},
            {{AEA_MUTATE, AEA_SCREAM, AEA_DIM, AEA_STAMINA_EMPTY}}
        },
        {
            translate_marker( "fractal" ),
            translate_marker( "has a self-similar pattern which repeats until it is too small for you to see" ),
            {{AEP_ALL_UP, AEP_ALL_UP, AEP_CLAIRVOYANCE, AEP_PSYSHIELD}},
            {{AEP_SCHIZO, AEP_ATTENTION, AEP_FORCE_TELEPORT, AEP_BAD_WEATHER}},
            {{AEA_STORM, AEA_FATIGUE, AEA_TELEPORT, AEA_FUN}},
            {{AEA_RADIATION, AEA_MUTATE, AEA_TELEGLOW, AEA_TELEGLOW}}
        }
    }
};
static const std::array<artifact_tool_form_datum, NUM_ARTTOOLFORMS> artifact_tool_form_data = { {
        {
            translate_marker( "Harp" ), ';', def_c_yellow, material_id( "wood" ), 5000_ml, 7500_ml, 1150_gram, 2100_gram, ARTWEAP_BULK,
            {{ARTWEAP_SPEAR, ARTWEAP_SWORD, ARTWEAP_KNIFE}}
        },

        {
            translate_marker( "Staff" ), '/', def_c_brown, material_id( "wood" ), 1500_ml, 3000_ml, 450_gram, 1150_gram, ARTWEAP_CLUB,
            {{ARTWEAP_BULK, ARTWEAP_SPEAR, ARTWEAP_KNIFE}}
        },

        {
            translate_marker( "Sword" ), '/', def_c_light_blue, material_id( "steel" ), 2000_ml, 3500_ml, 900_gram, 3259_gram, ARTWEAP_SWORD,
            {{ARTWEAP_BULK, NUM_ARTWEAPS, NUM_ARTWEAPS}}
        },

        {
            translate_marker( "Dagger" ), ';', def_c_light_blue, material_id( "steel" ), 250_ml, 1000_ml, 100_gram, 700_gram, ARTWEAP_KNIFE,
            {{NUM_ARTWEAPS, NUM_ARTWEAPS, NUM_ARTWEAPS}}
        },

        {
            translate_marker( "Cube" ), '*', def_c_white, material_id( "steel" ), 250_ml, 750_ml, 100_gram, 2300_gram, ARTWEAP_BULK,
            {{ARTWEAP_SPEAR, NUM_ARTWEAPS, NUM_ARTWEAPS}}
        }
    }
};
static const std::array<artifact_weapon_datum, NUM_ARTWEAPS> artifact_weapon_data = { {
        // Adjective      Vol   Weight    Bashing Cutting Stabbing To-hit Flag
        { translate_marker( "Heavy" ),   0_ml,   1400_gram, 10, 20,  0,  0,  0,  0, -2, 0, "" },
        { translate_marker( "Knobbed" ), 250_ml,  250_gram, 14, 30,  0,  0,  0,  0, -1, 1, "" },
        { translate_marker( "Spiked" ),  250_ml,  100_gram,  0,  0,  0,  0, 20, 40, -1, 1, "" },
        { translate_marker( "Edged" ),   500_ml,  450_gram,  0,  0, 20, 50,  0,  0, -1, 2, "SHEATH_SWORD" },
        { translate_marker( "Bladed" ),  250_ml, 2250_gram,  0,  0,  0,  0, 12, 30, -1, 1, "SHEATH_KNIFE" }
    }
};
static const std::array<artifact_armor_form_datum, NUM_ARTARMFORMS> artifact_armor_form_data = { {
        // Name    color  Material         Vol Wgt Enc MaxEnc Cov Thk Env Wrm Sto Bsh Cut Hit
        {
            translate_marker( "Robe" ),   def_c_red, material_id( "wool" ),    1500_ml, 700_gram,  1,  1,  90,  3,  0,  2,  0_ml, -8,  0, -3,
            { { bp_torso, bp_leg_l, bp_leg_r } }, false,
            {{
                    ARMORMOD_LIGHT, ARMORMOD_BULKY, ARMORMOD_POCKETED, ARMORMOD_FURRED,
                    ARMORMOD_PADDED
                }
            }
        },

        {
            translate_marker( "Coat" ),   def_c_brown, material_id( "leather" ),   3500_ml, 1600_gram,  2,  2,  80, 2,  1,  4,  1000_ml, -6,  0, -3,
            { bp_torso }, false,
            {{
                    ARMORMOD_LIGHT, ARMORMOD_POCKETED, ARMORMOD_FURRED, ARMORMOD_PADDED,
                    ARMORMOD_PLATED
                }
            }
        },

        {
            translate_marker( "Mask" ),   def_c_white, material_id( "wood" ),      1000_ml, 100_gram,  2,  2,  50, 2,  1,  2,  0_ml,  2,  0, -2,
            { { bp_eyes, bp_mouth } }, false,
            {{
                    ARMORMOD_FURRED, ARMORMOD_FURRED, ARMORMOD_NULL, ARMORMOD_NULL,
                    ARMORMOD_NULL
                }
            }
        },

        // Name    color  Materials             Vol  Wgt Enc MaxEnc Cov Thk Env Wrm Sto Bsh Cut Hit
        {
            translate_marker( "Helm" ),   def_c_dark_gray, material_id( "silver" ),    1500_ml, 700_gram,  2,  2,  85, 3,  0,  1,  0_ml,  8,  0, -2,
            { bp_head }, false,
            {{
                    ARMORMOD_BULKY, ARMORMOD_FURRED, ARMORMOD_PADDED, ARMORMOD_PLATED,
                    ARMORMOD_NULL
                }
            }
        },

        {
            translate_marker( "Gloves" ), def_c_light_blue, material_id( "leather" ), 500_ml, 100_gram,  1,  1,  90,  3,  1,  2,  0_ml, -4,  0, -2,
            { { bp_hand_l, bp_hand_r } }, true,
            {{
                    ARMORMOD_BULKY, ARMORMOD_FURRED, ARMORMOD_PADDED, ARMORMOD_PLATED,
                    ARMORMOD_NULL
                }
            }
        },

        // Name    color  Materials            Vol  Wgt Enc MaxEnc Cov Thk Env Wrm Sto Bsh Cut Hit
        {
            translate_marker( "Boots" ), def_c_blue, material_id( "leather" ),     1500_ml, 250_gram,  1,  1,  75,  3,  1,  3,  0_ml,  4,  0, -1,
            { { bp_foot_l, bp_foot_r } }, true,
            {{
                    ARMORMOD_LIGHT, ARMORMOD_BULKY, ARMORMOD_PADDED, ARMORMOD_PLATED,
                    ARMORMOD_NULL
                }
            }
        },

        {
            translate_marker( "Ring" ), def_c_light_green, material_id( "silver" ),   0_ml,  4_gram,  0,  0,  0,  0,  0,  0,  0_ml,  0,  0,  0,
            {}, false,
            {{ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}}
        }
    }
};
/*
 * Armor mods alter the normal values of armor.
 * If the basic armor type has "null" as its second material, and the mod has a
 * material attached, the second material will be changed.
 */
static const std::array<artifact_armor_form_datum, NUM_ARMORMODS> artifact_armor_mod_data = { {
        {
            "", def_c_white, material_id( "null" ), 0_ml,  0_gram,  0,  0,  0,  0,  0,  0,  0_ml,  0, 0, 0, {}, false,
            {{ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}}
        },
        // Description; "It is ..." or "They are ..."
        {
            translate_marker( "very thin and light." ), def_c_white, material_id( "null" ),
            // Vol   Wgt Enc MaxEnc Cov Thk Env Wrm Sto
            -1000_ml, -950_gram, -2, -2, -1, -1, -1, -1,  0_ml, 0, 0, 0, {},  false,
            {{ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}}
        },

        {
            translate_marker( "extremely bulky." ), def_c_white, material_id( "null" ),
            2000_ml, 1150_gram,  2,  2,  1,  1,  0,  1,  0_ml, 0, 0, 0, {},  false,
            {{ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}}
        },

        {
            translate_marker( "covered in pockets." ), def_c_white, material_id( "null" ),
            250_ml, 150_gram,  1,  1,  0,  0,  0,  0, 4000_ml, 0, 0, 0, {},  false,
            {{ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}}
        },

        {
            translate_marker( "disgustingly furry." ), def_c_white, material_id( "wool" ),
            // Vol  Wgt Enc MaxEnc Dmg Cut Env Wrm Sto
            1000_ml, 250_gram,  1,  1,  1,  1,  1,  3,  0_ml, 0, 0, 0, {},  false,
            {{ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}}
        },

        {
            translate_marker( "leather-padded." ), def_c_white, material_id( "leather" ),
            1000_ml, 450_gram,  1,  1,  1,  1,  0,  1, -750_ml, 0, 0, 0, {},  false,
            {{ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}}
        },

        {
            translate_marker( "plated in iron." ), def_c_white, material_id( "iron" ),
            1000_ml, 1400_gram,  3,  3,  2,  2,  0,  1, -1000_ml, 0, 0, 0, {}, false,
            {{ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL, ARMORMOD_NULL}}
        },
    }
};
static const std::array<std::string, 20> artifact_adj = { {
        translate_marker( "Forbidden" ), translate_marker( "Unknowable" ), translate_marker( "Forgotten" ), translate_marker( "Hideous" ), translate_marker( "Eldritch" ),
        translate_marker( "Gelatinous" ), translate_marker( "Ancient" ), translate_marker( "Cursed" ), translate_marker( "Bloody" ), translate_marker( "Undying" ),
        translate_marker( "Shadowy" ), translate_marker( "Silent" ), translate_marker( "Cyclopean" ), translate_marker( "Fungal" ), translate_marker( "Unspeakable" ),
        translate_marker( "Antediluvian" ), translate_marker( "Frigid" ), translate_marker( "Shattered" ), translate_marker( "Sleeping" ), translate_marker( "Repellent" )
    }
};
static const std::array<std::string, 20> artifact_noun = { {
        translate_marker( "%s Technique" ), translate_marker( "%s Dreams" ), translate_marker( "%s Beasts" ), translate_marker( "%s Evil" ), translate_marker( "%s Miasma" ),
        translate_marker( "the %s Abyss" ), translate_marker( "the %s City" ), translate_marker( "%s Shadows" ), translate_marker( "%s Shade" ), translate_marker( "%s Illusion" ),
        translate_marker( "%s Justice" ), translate_marker( "the %s Necropolis" ), translate_marker( "%s Ichor" ), translate_marker( "the %s Monolith" ), translate_marker( "%s Aeons" ),
        translate_marker( "%s Graves" ), translate_marker( "%s Horrors" ), translate_marker( "%s Suffering" ), translate_marker( "%s Death" ), translate_marker( "%s Horror" )
    }
};
std::string artifact_name( const std::string &type );
//Dreams for each charge req
static const std::array<artifact_dream_datum, NUM_ACRS> artifact_dream_data = { {
        {   {translate_marker( "The %s is somehow vaguely dissatisfied even though it doesn't want anything. Seeing this is a bug!" )},
            {translate_marker( "The %s is satisfied, as it should be because it has no standards. Seeing this is a bug!" )},
            100,  0
        }, { {translate_marker( "Your %s feels needy, like it wants to be held." )},
            {translate_marker( "You snuggle your %s closer." )},
            50,  35
        }, { {translate_marker( "Your %s feels needy, like it wants to be touched." )},
            {translate_marker( "You press your %s against your skin." )},
            50,  35
        }, { {translate_marker( "The %s is confused to find you dreaming while awake. Seeing this is a bug!" )},
            {translate_marker( "Your %s sleeps soundly." )},
            100, 33
        }, { {translate_marker( "Your %s longs for the glow." )},
            {translate_marker( "Your %s basks in the glow." )},
            25,  75
        }, { {translate_marker( "You dream of angels' tears falling on your %s." )},
            {translate_marker( "You dream of playing in the rain with your %s." )},
            50,  60
        }, { {translate_marker( "You dream that your %s is being buried alive." )},
            {translate_marker( "You dream of your %s dancing in a blue void." )},
            50,  50
        }
    }
};

// Constructors for artifact itypes.
it_artifact_tool::it_artifact_tool()
{
    tool.emplace();
    artifact.emplace();
    id = item_controller->create_artifact_id();
    price = 0_cent;
    tool->charges_per_use = 1;
    artifact->charge_type = ARTC_NULL;
    artifact->charge_req = ACR_NULL;
    artifact->dream_msg_unmet  = artifact_dream_data[static_cast<int>( ACR_NULL )].msg_unmet;
    artifact->dream_msg_met    = artifact_dream_data[static_cast<int>( ACR_NULL )].msg_met;
    artifact->dream_freq_unmet = artifact_dream_data[static_cast<int>( ACR_NULL )].freq_unmet;
    artifact->dream_freq_met   = artifact_dream_data[static_cast<int>( ACR_NULL )].freq_met;
    use_methods.emplace( "ARTIFACT", use_function( "ARTIFACT", &iuse::artifact ) );
}

it_artifact_tool::it_artifact_tool( JsonObject &jo )
{
    tool.emplace();
    artifact.emplace();
    use_methods.emplace( "ARTIFACT", use_function( "ARTIFACT", &iuse::artifact ) );
    deserialize( jo );
}

it_artifact_armor::it_artifact_armor()
{
    armor.emplace();
    artifact.emplace();
    id = item_controller->create_artifact_id();
    price = 0_cent;
}

it_artifact_armor::it_artifact_armor( JsonObject &jo )
{
    armor.emplace();
    artifact.emplace();
    deserialize( jo );
}

void it_artifact_tool::create_name( const std::string &type )
{
    name = no_translation( artifact_name( type ) );
}

void it_artifact_tool::create_name( const std::string &property_name,
                                    const std::string &shape_name )
{
    name = no_translation( string_format( pgettext( "artifact name (property, shape)", "%1$s %2$s" ),
                                          property_name, shape_name ) );
}

void it_artifact_armor::create_name( const std::string &type )
{
    name = no_translation( artifact_name( type ) );
}

std::string new_artifact()
{
    if( one_in( 2 ) ) { // Generate a "tool" artifact

        it_artifact_tool def;

        const artifact_tool_form_datum &info = random_entry_ref( artifact_tool_form_data );
        def.create_name( _( info.name ) );
        def.color = info.color;
        def.sym = std::string( 1, info.sym );
        def.materials.push_back( info.material );
        def.volume = rng( info.volume_min, info.volume_max );
        def.weight = rng( info.weight_min, info.weight_max );
        // Set up the basic weapon type
        const artifact_weapon_datum &weapon = artifact_weapon_data[info.base_weapon];
        def.melee[DT_BASH] = rng( weapon.bash_min, weapon.bash_max );
        def.melee[DT_CUT] = rng( weapon.cut_min, weapon.cut_max );
        def.melee[DT_STAB] = rng( weapon.stab_min, weapon.stab_max );
        def.m_to_hit = rng( weapon.to_hit_min, weapon.to_hit_max );
        if( !weapon.tag.empty() ) {
            def.item_tags.insert( weapon.tag );
        }
        // Add an extra weapon perhaps?
        if( one_in( 2 ) ) {
            const artifact_weapon_type select = random_entry_ref( info.extra_weapons );
            if( select != NUM_ARTWEAPS ) {
                const artifact_weapon_datum &weapon = artifact_weapon_data[select];
                def.volume += weapon.volume;
                def.weight += weapon.weight;
                def.melee[DT_BASH] += rng( weapon.bash_min, weapon.bash_max );
                def.melee[DT_CUT] += rng( weapon.cut_min, weapon.cut_max );
                def.melee[DT_STAB] += rng( weapon.stab_min, weapon.stab_max );
                def.m_to_hit += rng( weapon.to_hit_min, weapon.to_hit_max );
                if( !weapon.tag.empty() ) {
                    def.item_tags.insert( weapon.tag );
                }
                std::ostringstream newname;
                newname << _( weapon.adjective ) << " " << _( info.name );
                def.create_name( newname.str() );
            }
        }
        def.description = no_translation(
                              string_format(
                                  _( "This is the %s.\nIt is the only one of its kind.\nIt may have unknown powers; try activating them." ),
                                  def.nname( 1 ) ) );

        // Finally, pick some powers
        art_effect_passive passive_tmp = AEP_NULL;
        art_effect_active active_tmp = AEA_NULL;
        int num_good = 0;
        int num_bad = 0;
        int value = 0;
        std::vector<art_effect_passive> good_effects = fill_good_passive();
        std::vector<art_effect_passive> bad_effects = fill_bad_passive();

        // Wielded effects first
        while( !good_effects.empty() && !bad_effects.empty() &&
               num_good < 3 && num_bad < 3 &&
               ( num_good < 1 || num_bad < 1 || one_in( num_good + 1 ) ||
                 one_in( num_bad + 1 ) || value > 1 ) ) {
            if( value < 1 && one_in( 2 ) ) { // Good
                passive_tmp = random_entry_removed( good_effects );
                num_good++;
            } else if( !bad_effects.empty() ) { // Bad effect
                passive_tmp = random_entry_removed( bad_effects );
                num_bad++;
            }
            value += passive_effect_cost[passive_tmp];
            def.artifact->effects_wielded.push_back( passive_tmp );
        }
        // Next, carried effects; more likely to be just bad
        num_good = 0;
        num_bad = 0;
        value = 0;
        good_effects = fill_good_passive();
        bad_effects = fill_bad_passive();
        while( one_in( 2 ) && !good_effects.empty() && !bad_effects.empty() &&
               num_good < 3 && num_bad < 3 &&
               ( ( num_good > 2 && one_in( num_good + 1 ) ) || num_bad < 1 ||
                 one_in( num_bad + 1 ) || value > 1 ) ) {
            if( value < 1 && one_in( 3 ) ) { // Good
                passive_tmp = random_entry_removed( good_effects );
                num_good++;
            } else { // Bad effect
                passive_tmp = random_entry_removed( bad_effects );
                num_bad++;
            }
            value += passive_effect_cost[passive_tmp];
            def.artifact->effects_carried.push_back( passive_tmp );
        }
        // Finally, activated effects; not necessarily good or bad
        num_good = 0;
        num_bad = 0;
        value = 0;
        std::vector<art_effect_active> good_a_effects = fill_good_active();
        std::vector<art_effect_active> bad_a_effects = fill_bad_active();
        while( !good_a_effects.empty() && !bad_a_effects.empty() &&
               num_good < 3 && num_bad < 3 &&
               ( value > 3 || ( num_bad > 0 && num_good == 0 ) ||
                 !one_in( 3 - num_good ) || !one_in( 3 - num_bad ) ) ) {
            if( !one_in( 3 ) && value <= 1 ) { // Good effect
                active_tmp = random_entry_removed( good_a_effects );
                num_good++;
                value += active_effect_cost[active_tmp];
            } else { // Bad effect
                active_tmp = random_entry_removed( bad_a_effects );
                num_bad++;
                value += active_effect_cost[active_tmp];
            }
            def.artifact->effects_activated.push_back( active_tmp );
            def.tool->max_charges += rng( 1, 3 );
        }
        def.tool->def_charges = def.tool->max_charges;
        // If we have charges, pick a recharge mechanism
        if( def.tool->max_charges > 0 ) {
            def.artifact->charge_type = static_cast<art_charge>( rng( ARTC_NULL + 1, NUM_ARTCS - 1 ) );
        }
        if( one_in( 8 ) && num_bad + num_good >= 4 ) {
            def.artifact->charge_type = ARTC_NULL;    // 1 in 8 chance that it can't recharge!
        }
        // Maybe pick an extra recharge requirement
        if( one_in( std::max( 1, 6 - num_good ) ) && def.artifact->charge_type != ARTC_NULL ) {
            def.artifact->charge_req = static_cast<art_charge_req>( rng( ACR_NULL + 1, NUM_ACRS - 1 ) );
        }
        // Assign dream data (stored individually so they can be overridden in json)
        def.artifact->dream_msg_unmet  = artifact_dream_data[static_cast<int>
                                         ( def.artifact->charge_req )].msg_unmet;
        def.artifact->dream_msg_met    = artifact_dream_data[static_cast<int>
                                         ( def.artifact->charge_req )].msg_met;
        def.artifact->dream_freq_unmet = artifact_dream_data[static_cast<int>
                                         ( def.artifact->charge_req )].freq_unmet;
        def.artifact->dream_freq_met   = artifact_dream_data[static_cast<int>
                                         ( def.artifact->charge_req )].freq_met;
        // Stronger artifacts have a higher chance of picking their dream
        def.artifact->dream_freq_unmet *= ( 1 + 0.1 * ( num_bad + num_good ) );
        def.artifact->dream_freq_met   *= ( 1 + 0.1 * ( num_bad + num_good ) );

        item_controller->add_item_type( static_cast<itype &>( def ) );
        return def.get_id();
    } else { // Generate an armor artifact

        it_artifact_armor def;

        const artifact_armor_form_datum &info = random_entry_ref( artifact_armor_form_data );

        def.create_name( _( info.name ) );
        def.sym = "["; // Armor is always [
        def.color = info.color;
        def.materials.push_back( info.material );
        def.volume = info.volume;
        def.weight = info.weight;
        def.melee[DT_BASH] = info.melee_bash;
        def.melee[DT_CUT] = info.melee_cut;
        def.m_to_hit = info.melee_hit;
        def.armor->covers = info.covers;
        def.armor->encumber = info.encumb;
        def.armor->max_encumber = info.max_encumb;
        def.armor->coverage = info.coverage;
        def.armor->thickness = info.thickness;
        def.armor->env_resist = info.env_resist;
        def.armor->warmth = info.warmth;
        def.armor->storage = info.storage;
        std::ostringstream description;
        description << string_format( info.plural ?
                                      _( "This is the %s.\nThey are the only ones of their kind." ) :
                                      _( "This is the %s.\nIt is the only one of its kind." ),
                                      def.nname( 1 ) );

        // Modify the armor further
        if( !one_in( 4 ) ) {
            const artifact_armor_mod mod = random_entry_ref( info.available_mods );
            if( mod != ARMORMOD_NULL ) {
                const artifact_armor_form_datum &modinfo = artifact_armor_mod_data[mod];
                if( modinfo.volume >= 0_ml || def.volume > -modinfo.volume ) {
                    def.volume += modinfo.volume;
                } else {
                    def.volume = 250_ml;
                }

                if( modinfo.weight >= 0_gram || def.weight.value() > std::abs( modinfo.weight.value() ) ) {
                    def.weight += modinfo.weight;
                } else {
                    def.weight = 1_gram;
                }

                def.armor->encumber += modinfo.encumb;

                if( modinfo.coverage > 0 || def.armor->coverage > std::abs( modinfo.coverage ) ) {
                    def.armor->coverage += modinfo.coverage;
                } else {
                    def.armor->coverage = 0;
                }

                if( modinfo.thickness > 0 || def.armor->thickness > std::abs( modinfo.thickness ) ) {
                    def.armor->thickness += modinfo.thickness;
                } else {
                    def.armor->thickness = 0;
                }

                if( modinfo.env_resist > 0 || def.armor->env_resist > std::abs( modinfo.env_resist ) ) {
                    def.armor->env_resist += modinfo.env_resist;
                } else {
                    def.armor->env_resist = 0;
                }
                def.armor->warmth += modinfo.warmth;

                if( modinfo.storage > 0_ml || def.armor->storage > -modinfo.storage ) {
                    def.armor->storage += modinfo.storage;
                } else {
                    def.armor->storage = 0_ml;
                }

                description << string_format( info.plural ?
                                              _( "\nThey are %s" ) :
                                              _( "\nIt is %s" ),
                                              _( modinfo.name ) );
            }
        }

        def.description = no_translation( description.str() );

        // Finally, pick some effects
        int num_good = 0;
        int num_bad = 0;
        int value = 0;
        art_effect_passive passive_tmp = AEP_NULL;
        std::vector<art_effect_passive> good_effects = fill_good_passive();
        std::vector<art_effect_passive> bad_effects = fill_bad_passive();

        while( !good_effects.empty() && !bad_effects.empty() &&
               num_good < 3 && num_bad < 3 &&
               ( num_good < 1 || one_in( num_good * 2 ) || value > 1 ||
                 ( num_bad < 3 && !one_in( 3 - num_bad ) ) ) ) {
            if( value < 1 && one_in( 2 ) ) { // Good effect
                passive_tmp = random_entry_removed( good_effects );
                num_good++;
            } else { // Bad effect
                passive_tmp = random_entry_removed( bad_effects );
                num_bad++;
            }
            value += passive_effect_cost[passive_tmp];
            def.artifact->effects_worn.push_back( passive_tmp );
        }
        item_controller->add_item_type( static_cast<itype &>( def ) );
        return def.get_id();
    }
}

std::string new_natural_artifact( artifact_natural_property prop )
{
    // Natural artifacts are always tools.
    it_artifact_tool def;

    // Pick a form
    const artifact_shape_datum &shape_data = random_entry_ref( artifact_shape_data );
    // Pick a property
    const artifact_natural_property property = ( prop > ARTPROP_NULL ? prop :
            static_cast<artifact_natural_property>( rng( ARTPROP_NULL + 1,
                    ARTPROP_MAX - 1 ) ) );
    const artifact_property_datum &property_data = artifact_property_data[property];

    def.sym = ":";
    def.color = c_yellow;
    def.materials.push_back( material_id( "stone" ) );
    def.volume = rng( shape_data.volume_min, shape_data.volume_max );
    def.weight = rng( shape_data.weight_min, shape_data.weight_max );
    def.melee[DT_BASH] = 0;
    def.melee[DT_CUT] = 0;
    def.m_to_hit = 0;

    def.create_name( _( property_data.name ), _( shape_data.name ) );
    def.description = no_translation(
                          string_format( pgettext( "artifact description", "This %1$s %2$s." ),
                                         _( shape_data.desc ), _( property_data.desc ) ) );

    // Three possibilities: good passive + bad passive, good active + bad active,
    // and bad passive + good active
    bool good_passive = false, bad_passive = false,
         good_active  = false, bad_active  = false;
    switch( rng( 1, 3 ) ) {
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
        if( good_passive ) {
            aep_good = random_entry_ref( property_data.passive_good );
            if( aep_good == AEP_NULL || one_in( 4 ) ) {
                aep_good = static_cast<art_effect_passive>( rng( AEP_NULL + 1, AEP_SPLIT - 1 ) );
            }
        }
        if( bad_passive ) {
            aep_bad = random_entry_ref( property_data.passive_bad );
            if( aep_bad == AEP_NULL || one_in( 4 ) ) {
                aep_bad = static_cast<art_effect_passive>( rng( AEP_SPLIT + 1, NUM_AEAS - 1 ) );
            }
        }
        if( good_active ) {
            aea_good = random_entry_ref( property_data.active_good );
            if( aea_good == AEA_NULL || one_in( 4 ) ) {
                aea_good = static_cast<art_effect_active>( rng( AEA_NULL + 1, AEA_SPLIT - 1 ) );
            }
        }
        if( bad_active ) {
            aea_bad = random_entry_ref( property_data.active_bad );
            if( aea_bad == AEA_NULL || one_in( 4 ) ) {
                aea_bad = static_cast<art_effect_active>( rng( AEA_SPLIT + 1, NUM_AEAS - 1 ) );
            }
        }

        value = passive_effect_cost[aep_good] + passive_effect_cost[aep_bad] +
                active_effect_cost[aea_good] +  active_effect_cost[aea_bad];
        value_to_reach++; // Yes, it is intentional that this is 1 the first check
    } while( value > value_to_reach );

    if( aep_good != AEP_NULL ) {
        def.artifact->effects_carried.push_back( aep_good );
    }
    if( aep_bad != AEP_NULL ) {
        def.artifact->effects_carried.push_back( aep_bad );
    }
    if( aea_good != AEA_NULL ) {
        def.artifact->effects_activated.push_back( aea_good );
    }
    if( aea_bad != AEA_NULL ) {
        def.artifact->effects_activated.push_back( aea_bad );
    }

    // Natural artifacts ALWAYS can recharge
    // (When "implanting" them in a mundane item, this ability may be lost
    if( !def.artifact->effects_activated.empty() ) {
        def.tool->def_charges = def.tool->max_charges = rng( 1, 4 );
        def.artifact->charge_type = static_cast<art_charge>( rng( ARTC_NULL + 1, NUM_ARTCS - 1 ) );
        //Maybe pick an extra recharge requirement
        if( one_in( 6 ) ) {
            def.artifact->charge_req = static_cast<art_charge_req>( rng( ACR_NULL + 1, NUM_ACRS - 1 ) );
        }
    }
    // Assign dream data (stored individually so they can be overridden in json)
    def.artifact->dream_msg_unmet  = artifact_dream_data[static_cast<int>
                                     ( def.artifact->charge_req )].msg_unmet;
    def.artifact->dream_msg_met    = artifact_dream_data[static_cast<int>
                                     ( def.artifact->charge_req )].msg_met;
    def.artifact->dream_freq_unmet = artifact_dream_data[static_cast<int>
                                     ( def.artifact->charge_req )].freq_unmet;
    def.artifact->dream_freq_met   = artifact_dream_data[static_cast<int>
                                     ( def.artifact->charge_req )].freq_met;
    item_controller->add_item_type( static_cast<itype &>( def ) );
    return def.get_id();
}

// Make a special debugging artifact.
std::string architects_cube()
{
    it_artifact_tool def;

    const artifact_tool_form_datum &info = artifact_tool_form_data[ARTTOOLFORM_CUBE];
    def.create_name( _( info.name ) );
    def.color = info.color;
    def.sym = std::string( 1, info.sym );
    def.materials.push_back( info.material );
    def.volume = rng( info.volume_min, info.volume_max );
    def.weight = rng( info.weight_min, info.weight_max );
    // Set up the basic weapon type
    const artifact_weapon_datum &weapon = artifact_weapon_data[info.base_weapon];
    def.melee[DT_BASH] = rng( weapon.bash_min, weapon.bash_max );
    def.melee[DT_CUT] = rng( weapon.cut_min, weapon.cut_max );
    def.m_to_hit = rng( weapon.to_hit_min, weapon.to_hit_max );
    if( !weapon.tag.empty() ) {
        def.item_tags.insert( weapon.tag );
    }
    // Add an extra weapon perhaps?
    // Most artifact descriptions are generated and stored using `no_translation`,
    // also do it here for consistency
    def.description = no_translation( _( "The architect's cube." ) );
    def.artifact->effects_carried.push_back( AEP_SUPER_CLAIRVOYANCE );
    item_controller->add_item_type( static_cast<itype &>( def ) );
    return def.get_id();
}

std::vector<art_effect_passive> fill_good_passive()
{
    std::vector<art_effect_passive> ret;
    for( int i = AEP_NULL + 1; i < AEP_SPLIT; i++ ) {
        ret.push_back( static_cast<art_effect_passive>( i ) );
    }
    return ret;
}

std::vector<art_effect_passive> fill_bad_passive()
{
    std::vector<art_effect_passive> ret;
    for( int i = AEP_SPLIT + 1; i < NUM_AEPS; i++ ) {
        ret.push_back( static_cast<art_effect_passive>( i ) );
    }
    return ret;
}

std::vector<art_effect_active> fill_good_active()
{
    std::vector<art_effect_active> ret;
    for( int i = AEA_NULL + 1; i < AEA_SPLIT; i++ ) {
        ret.push_back( static_cast<art_effect_active>( i ) );
    }
    return ret;
}

std::vector<art_effect_active> fill_bad_active()
{
    std::vector<art_effect_active> ret;
    for( int i = AEA_SPLIT + 1; i < NUM_AEAS; i++ ) {
        ret.push_back( static_cast<art_effect_active>( i ) );
    }
    return ret;
}

std::string artifact_name( const std::string &type )
{
    const std::string noun = _( random_entry_ref( artifact_noun ) );
    const std::string adj = _( random_entry_ref( artifact_adj ) );
    std::string ret = string_format( noun, adj );
    ret = string_format( pgettext( "artifact name (type, noun)", "%1$s of %2$s" ), type, ret );
    return ret;
}

/* Json Loading and saving */

void load_artifacts( const std::string &path )
{
    read_from_file_optional_json( path, []( JsonIn & artifact_json ) {
        artifact_json.start_array();
        while( !artifact_json.end_array() ) {
            JsonObject jo = artifact_json.get_object();
            std::string type = jo.get_string( "type" );
            if( type == "artifact_tool" ) {
                item_controller->add_item_type( static_cast<const itype &>( it_artifact_tool( jo ) ) );
            } else if( type == "artifact_armor" ) {
                item_controller->add_item_type( static_cast<const itype &>( it_artifact_armor( jo ) ) );
            } else {
                jo.throw_error( "unrecognized artifact type.", "type" );
            }
        }
    } );
}

void it_artifact_tool::deserialize( JsonObject &jo )
{
    id = jo.get_string( "id" );
    name = no_translation( jo.get_string( "name" ) );
    description = no_translation( jo.get_string( "description" ) );
    if( jo.has_int( "sym" ) ) {
        sym = std::string( 1, jo.get_int( "sym" ) );
    } else {
        sym = jo.get_string( "sym" );
    }
    jo.read( "color", color );
    assign( jo, "price", price, false, 0_cent );
    // LEGACY: Since it seems artifacts get serialized out to disk, and they're
    // dynamic, we need to allow for them to be read from disk for, oh, I guess
    // quite some time. Loading and saving once will write things out as a JSON
    // array.
    if( jo.has_string( "m1" ) ) {
        materials.push_back( material_id( jo.get_string( "m1" ) ) );
    }
    if( jo.has_string( "m2" ) ) {
        materials.push_back( material_id( jo.get_string( "m2" ) ) );
    }
    // Assumption, perhaps dangerous, that we won't wind up with m1 and m2 and
    // a materials array in our serialized objects at the same time.
    if( jo.has_array( "materials" ) ) {
        JsonArray jarr = jo.get_array( "materials" );
        for( size_t i = 0; i < jarr.size(); ++i ) {
            materials.push_back( material_id( jarr.get_string( i ) ) );
        }
    }
    volume = jo.get_int( "volume" ) * units::legacy_volume_factor;
    weight = units::from_gram<std::int64_t>( jo.get_int( "weight" ) );
    melee[DT_BASH] = jo.get_int( "melee_dam" );
    melee[DT_CUT] = jo.get_int( "melee_cut" );
    m_to_hit = jo.get_int( "m_to_hit" );
    item_tags = jo.get_tags( "item_flags" );

    tool->max_charges = jo.get_int( "max_charges" );
    tool->def_charges = jo.get_int( "def_charges" );

    tool->charges_per_use = jo.get_int( "charges_per_use" );
    tool->turns_per_charge = jo.get_int( "turns_per_charge" );

    // Artifacts in older saves store ammo as string.
    if( jo.has_array( "ammo" ) ) {
        JsonArray atypes = jo.get_array( "ammo" );
        for( size_t i = 0; i < atypes.size(); ++i ) {
            tool->ammo_id.insert( ammotype( atypes.get_string( i ) ) );
        }
    } else if( jo.has_string( "ammo" ) ) {
        tool->ammo_id.insert( ammotype( jo.get_string( "ammo" ) ) );
    } else {
        jo.throw_error( "\"ammo\" node is neither array, not string" );
    }

    tool->revert_to.emplace( jo.get_string( "revert_to", "null" ) );
    if( *tool->revert_to == "null" ) {
        tool->revert_to.reset();
    }

    artifact->charge_type = static_cast<art_charge>( jo.get_int( "charge_type" ) );

    // Artifacts in older saves do not have charge_req
    if( jo.has_int( "charge_req" ) ) {
        artifact->charge_req  = static_cast<art_charge_req>( jo.get_int( "charge_req" ) );
    } else {
        artifact->charge_req = ACR_NULL;
    }

    JsonArray ja = jo.get_array( "effects_wielded" );
    while( ja.has_more() ) {
        artifact->effects_wielded.push_back( static_cast<art_effect_passive>( ja.next_int() ) );
    }

    ja = jo.get_array( "effects_activated" );
    while( ja.has_more() ) {
        artifact->effects_activated.push_back( static_cast<art_effect_active>( ja.next_int() ) );
    }

    ja = jo.get_array( "effects_carried" );
    while( ja.has_more() ) {
        artifact->effects_carried.push_back( static_cast<art_effect_passive>( ja.next_int() ) );
    }

    //Generate any missing dream data (due to e.g. old save)
    if( !jo.has_array( "dream_unmet" ) ) {
        artifact->dream_msg_unmet = artifact_dream_data[static_cast<int>( artifact->charge_req )].msg_unmet;
    } else {
        ja = jo.get_array( "dream_unmet" );
        while( ja.has_more() ) {
            artifact->dream_msg_unmet.push_back( ja.next_string() );
        }
    }
    if( !jo.has_array( "dream_met" ) ) {
        artifact->dream_msg_met   = artifact_dream_data[static_cast<int>( artifact->charge_req )].msg_met;
    } else {
        ja = jo.get_array( "dream_met" );
        while( ja.has_more() ) {
            artifact->dream_msg_met.push_back( ja.next_string() );
        }
    }
    if( jo.has_int( "dream_freq_unmet" ) ) {
        artifact->dream_freq_unmet = jo.get_int( "dream_freq_unmet" );
    } else {
        artifact->dream_freq_unmet = artifact_dream_data[static_cast<int>
                                     ( artifact->charge_req )].freq_unmet;
    }
    if( jo.has_int( "dream_freq_met" ) ) {
        artifact->dream_freq_met   = jo.get_int( "dream_freq_met" );
    } else {
        artifact->dream_freq_met   = artifact_dream_data[static_cast<int>( artifact->charge_req )].freq_met;
    }

}

void it_artifact_armor::deserialize( JsonObject &jo )
{
    id = jo.get_string( "id" );
    name = no_translation( jo.get_string( "name" ) );
    description = no_translation( jo.get_string( "description" ) );
    if( jo.has_int( "sym" ) ) {
        sym = std::string( 1, jo.get_int( "sym" ) );
    } else {
        sym = jo.get_string( "sym" );
    }
    jo.read( "color", color );
    assign( jo, "price", price, false, 0_cent );
    // LEGACY: Since it seems artifacts get serialized out to disk, and they're
    // dynamic, we need to allow for them to be read from disk for, oh, I guess
    // quite some time. Loading and saving once will write things out as a JSON
    // array.
    if( jo.has_string( "m1" ) ) {
        materials.push_back( material_id( jo.get_string( "m1" ) ) );
    }
    if( jo.has_string( "m2" ) ) {
        materials.push_back( material_id( jo.get_string( "m2" ) ) );
    }
    // Assumption, perhaps dangerous, that we won't wind up with m1 and m2 and
    // a materials array in our serialized objects at the same time.
    if( jo.has_array( "materials" ) ) {
        JsonArray jarr = jo.get_array( "materials" );
        for( size_t i = 0; i < jarr.size(); ++i ) {
            materials.push_back( material_id( jarr.get_string( i ) ) );
        }
    }
    volume = jo.get_int( "volume" ) * units::legacy_volume_factor;
    weight = units::from_gram<std::int64_t>( jo.get_int( "weight" ) );
    melee[DT_BASH] = jo.get_int( "melee_dam" );
    melee[DT_CUT] = jo.get_int( "melee_cut" );
    m_to_hit = jo.get_int( "m_to_hit" );
    item_tags = jo.get_tags( "item_flags" );

    jo.read( "covers", armor->covers );
    armor->encumber = jo.get_int( "encumber" );
    // Old saves don't have max_encumber, so set it to base encumbrance value
    armor->max_encumber = jo.get_int( "max_encumber", armor->encumber );
    armor->coverage = jo.get_int( "coverage" );
    armor->thickness = jo.get_int( "material_thickness" );
    armor->env_resist = jo.get_int( "env_resist" );
    armor->warmth = jo.get_int( "warmth" );
    armor->storage = jo.get_int( "storage" ) * units::legacy_volume_factor;
    armor->power_armor = jo.get_bool( "power_armor" );

    JsonArray ja = jo.get_array( "effects_worn" );
    while( ja.has_more() ) {
        artifact->effects_worn.push_back( static_cast<art_effect_passive>( ja.next_int() ) );
    }
}

bool save_artifacts( const std::string &path )
{
    return write_to_file( path, [&]( std::ostream & fout ) {
        JsonOut json( fout, true );
        json.start_array();
        // We only want runtime types, otherwise static artifacts are loaded twice (on init and then on game load)
        for( const itype *e : item_controller->get_runtime_types() ) {
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
void serialize_enum_vector_as_int( JsonOut &json, const std::string &member,
                                   const std::vector<E> &vec )
{
    json.member( member );
    json.start_array();
    for( auto &e : vec ) {
        json.write( static_cast<int>( e ) );
    }
    json.end_array();
}

void it_artifact_tool::serialize( JsonOut &json ) const
{
    json.start_object();

    json.member( "type", "artifact_tool" );

    // generic data
    json.member( "id", id );
    // Artifact names and descriptions are always constructed using `no_translation`,
    // so `translated()` here only retrieves the underlying string
    json.member( "name", name.translated() );
    json.member( "description", description.translated() );
    json.member( "sym", sym );
    json.member( "color", color );
    json.member( "price", units::to_cent( price ) );
    json.member( "materials" );
    json.start_array();
    for( const material_id &mat : materials ) {
        json.write( mat );
    }
    json.end_array();
    json.member( "volume", volume / units::legacy_volume_factor );
    json.member( "weight", to_gram( weight ) );

    json.member( "melee_dam", melee[DT_BASH] );
    json.member( "melee_cut", melee[DT_CUT] );

    json.member( "m_to_hit", m_to_hit );

    json.member( "item_flags", item_tags );
    json.member( "techniques", techniques );

    // tool data
    json.member( "ammo", tool->ammo_id );
    json.member( "max_charges", tool->max_charges );
    json.member( "def_charges", tool->def_charges );
    json.member( "charges_per_use", tool->charges_per_use );
    json.member( "turns_per_charge", tool->turns_per_charge );
    if( tool->revert_to ) {
        json.member( "revert_to", *tool->revert_to );
    }

    // artifact data
    json.member( "charge_type", artifact->charge_type );
    json.member( "charge_req",  artifact->charge_req );
    serialize_enum_vector_as_int( json, "effects_wielded", artifact->effects_wielded );
    serialize_enum_vector_as_int( json, "effects_activated", artifact->effects_activated );
    serialize_enum_vector_as_int( json, "effects_carried", artifact->effects_carried );
    json.member( "dream_unmet",        artifact->dream_msg_unmet );
    json.member( "dream_met",          artifact->dream_msg_met );
    json.member( "dream_freq_unmet",   artifact->dream_freq_unmet );
    json.member( "dream_freq_met",     artifact->dream_freq_met );

    json.end_object();
}

void it_artifact_armor::serialize( JsonOut &json ) const
{
    json.start_object();

    json.member( "type", "artifact_armor" );

    // generic data
    json.member( "id", id );
    // Artifact names and descriptions are always constructed using `no_translation`,
    // so `translated()` here only retrieves the underlying string
    json.member( "name", name.translated() );
    json.member( "description", description.translated() );
    json.member( "sym", sym );
    json.member( "color", color );
    json.member( "price", units::to_cent( price ) );
    json.member( "materials" );
    json.start_array();
    for( const material_id &mat : materials ) {
        json.write( mat );
    }
    json.end_array();
    json.member( "volume", volume / units::legacy_volume_factor );
    json.member( "weight", to_gram( weight ) );

    json.member( "melee_dam", melee[DT_BASH] );
    json.member( "melee_cut", melee[DT_CUT] );

    json.member( "m_to_hit", m_to_hit );

    json.member( "item_flags", item_tags );

    json.member( "techniques", techniques );

    // armor data
    json.member( "covers", armor->covers );
    json.member( "encumber", armor->encumber );
    json.member( "max_encumber", armor->max_encumber );
    json.member( "coverage", armor->coverage );
    json.member( "material_thickness", armor->thickness );
    json.member( "env_resist", armor->env_resist );
    json.member( "warmth", armor->warmth );
    json.member( "storage", armor->storage / units::legacy_volume_factor );
    json.member( "power_armor", armor->power_armor );

    // artifact data
    serialize_enum_vector_as_int( json, "effects_worn", artifact->effects_worn );

    json.end_object();
}

namespace io
{
#define PAIR(x) case x: return #x;
template<>
std::string enum_to_string<art_effect_passive>( art_effect_passive data )
{
    switch( data ) {
        // *INDENT-OFF*
        PAIR( AEP_NULL )
        PAIR( AEP_STR_UP )
        PAIR( AEP_DEX_UP )
        PAIR( AEP_PER_UP )
        PAIR( AEP_INT_UP )
        PAIR( AEP_ALL_UP )
        PAIR( AEP_SPEED_UP )
        PAIR( AEP_PBLUE )
        PAIR( AEP_SNAKES )
        PAIR( AEP_INVISIBLE )
        PAIR( AEP_CLAIRVOYANCE )
        PAIR( AEP_CLAIRVOYANCE_PLUS )
        PAIR( AEP_SUPER_CLAIRVOYANCE )
        PAIR( AEP_STEALTH )
        PAIR( AEP_EXTINGUISH )
        PAIR( AEP_GLOW )
        PAIR( AEP_PSYSHIELD )
        PAIR( AEP_RESIST_ELECTRICITY )
        PAIR( AEP_CARRY_MORE )
        PAIR( AEP_SAP_LIFE )
        PAIR( AEP_FUN )
        PAIR( AEP_SPLIT )
        PAIR( AEP_HUNGER )
        PAIR( AEP_THIRST )
        PAIR( AEP_SMOKE )
        PAIR( AEP_EVIL )
        PAIR( AEP_SCHIZO )
        PAIR( AEP_RADIOACTIVE )
        PAIR( AEP_MUTAGENIC )
        PAIR( AEP_ATTENTION )
        PAIR( AEP_STR_DOWN )
        PAIR( AEP_DEX_DOWN )
        PAIR( AEP_PER_DOWN )
        PAIR( AEP_INT_DOWN )
        PAIR( AEP_ALL_DOWN )
        PAIR( AEP_SPEED_DOWN )
        PAIR( AEP_FORCE_TELEPORT )
        PAIR( AEP_MOVEMENT_NOISE )
        PAIR( AEP_BAD_WEATHER )
        PAIR( AEP_SICK )
        // *INDENT-ON*
        case NUM_AEPS:
            break;
    }
    debugmsg( "Invalid AEP" );
    abort();
}
template<>
std::string enum_to_string<art_effect_active>( art_effect_active data )
{
    switch( data ) {
        // *INDENT-OFF*
        PAIR( AEA_NULL )
        PAIR( AEA_STORM )
        PAIR( AEA_FIREBALL )
        PAIR( AEA_ADRENALINE )
        PAIR( AEA_MAP )
        PAIR( AEA_BLOOD )
        PAIR( AEA_FATIGUE )
        PAIR( AEA_ACIDBALL )
        PAIR( AEA_PULSE )
        PAIR( AEA_HEAL )
        PAIR( AEA_CONFUSED )
        PAIR( AEA_ENTRANCE )
        PAIR( AEA_BUGS )
        PAIR( AEA_TELEPORT )
        PAIR( AEA_LIGHT )
        PAIR( AEA_GROWTH )
        PAIR( AEA_HURTALL )
        PAIR( AEA_FUN )
        PAIR( AEA_SPLIT )
        PAIR( AEA_RADIATION )
        PAIR( AEA_PAIN )
        PAIR( AEA_MUTATE )
        PAIR( AEA_PARALYZE )
        PAIR( AEA_FIRESTORM )
        PAIR( AEA_ATTENTION )
        PAIR( AEA_TELEGLOW )
        PAIR( AEA_NOISE )
        PAIR( AEA_SCREAM )
        PAIR( AEA_DIM )
        PAIR( AEA_FLASH )
        PAIR( AEA_VOMIT )
        PAIR( AEA_SHADOWS )
        PAIR( AEA_STAMINA_EMPTY )
        // *INDENT-ON*
        case NUM_AEAS:
            break;
    }
    debugmsg( "Invalid AEA" );
    abort();
}

template<>
std::string enum_to_string<art_charge>( art_charge data )
{
    switch( data ) {
        // *INDENT-OFF*
        PAIR( ARTC_NULL )
        PAIR( ARTC_TIME )
        PAIR( ARTC_SOLAR )
        PAIR( ARTC_PAIN )
        PAIR( ARTC_HP )
        PAIR( ARTC_FATIGUE )
        PAIR( ARTC_PORTAL )
        // *INDENT-ON*
        case NUM_ARTCS:
            break;
    }
    debugmsg( "Invalid ARTC" );
    abort();
}

template<>
std::string enum_to_string<art_charge_req>( art_charge_req data )
{
    switch( data ) {
        // *INDENT-OFF*
        PAIR( ACR_NULL )
        PAIR( ACR_EQUIP )
        PAIR( ACR_SKIN )
        PAIR( ACR_SLEEP )
        PAIR( ACR_RAD )
        PAIR( ACR_WET )
        PAIR( ACR_SKY )
        // *INDENT-ON*
        case NUM_ACRS:
            break;
    }
    debugmsg( "Invalid ACR" );
    abort();
}
#undef PAIR

} // namespace io
