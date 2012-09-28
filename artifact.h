#ifndef _ARTIFACT_H_
#define _ARTIFACT_H_

#include <string>
#include <vector>

enum art_effect_passive {
 AEP_NULL = 0,
// Good
 AEP_STR_UP, // Strength + 4
 AEP_DEX_UP, // Dexterity + 4
 AEP_PER_UP, // Perception + 4
 AEP_INT_UP, // Intelligence + 4
 AEP_ALL_UP, // All stats + 2
 AEP_SPEED_UP, // +20 speed
 AEP_IODINE, // Reduces radiation
 AEP_SNAKES, // Summons friendly snakes when you're hit
 AEP_INVISIBLE, // Makes you invisible
 AEP_CLAIRVOYANCE, // See through walls
 AEP_STEALTH, // Your steps are quieted
 AEP_EXTINGUISH, // May extinguish nearby flames
 AEP_GLOW, // Four-tile light source
 AEP_PSYSHIELD, // Protection from stare attacks
 AEP_RESIST_ELECTRICITY, // Protection from electricity
 AEP_CARRY_MORE, // Increases carrying capacity by 200
 AEP_SAP_LIFE, // Killing non-zombie monsters may heal you
// Splits good from bad
 AEP_SPLIT,
// Bad
 AEP_HUNGER, // Increases hunger
 AEP_THIRST, // Increases thirst
 AEP_SMOKE, // Emits smoke occasionally
 AEP_EVIL, // Addiction to the power
 AEP_SCHIZO, // Mimicks schizophrenia
 AEP_RADIOACTIVE, // Increases your radiation
 AEP_MUTAGENIC, // Mutates you slowly
 AEP_ATTENTION, // Draws netherworld attention slowly
 AEP_STR_DOWN, // Strength - 3
 AEP_DEX_DOWN, // Dex - 3
 AEP_PER_DOWN, // Per - 3
 AEP_INT_DOWN, // Int - 3
 AEP_ALL_DOWN, // All stats - 2
 AEP_SPEED_DOWN, // -20 speed
 AEP_FORCE_TELEPORT, // Occasionally force a teleport
 AEP_MOVEMENT_NOISE, // Makes noise when you move
 AEP_BAD_WEATHER, // More likely to experience bad weather
 AEP_SICK, // Decreases health

 NUM_AEPS
};

enum art_effect_active {
 AEA_NULL = 0,

 AEA_STORM, // Emits shock fields
 AEA_FIREBALL, // Targeted
 AEA_ADRENALINE, // Adrenaline rush
 AEA_MAP, // Maps the area around you
 AEA_BLOOD, // Shoots blood all over
 AEA_FATIGUE, // Creates interdimensional fatigue
 AEA_ACIDBALL, // Targeted acid
 AEA_PULSE, // Destroys adjacent terrain
 AEA_HEAL, // Heals minor damage
 AEA_CONFUSED, // Confuses all monsters in view
 AEA_ENTRANCE, // Chance to make nearby monsters friendly
 AEA_BUGS, // Chance to summon friendly insects
 AEA_TELEPORT, // Teleports you
 AEA_LIGHT, // Temporary light source
 AEA_GROWTH, // Grow plants, a la triffid queen
 AEA_HURTALL, // Hurts all monsters!

 AEA_SPLIT, // Split between good and bad

 AEA_RADIATION, // Spew radioactive gas
 AEA_PAIN, // Increases player pain
 AEA_MUTATE, // Chance of mutation
 AEA_PARALYZE, // You lose several turns
 AEA_FIRESTORM, // Spreads minor fire all around you
 AEA_ATTENTION, // Attention from sub-prime denizens
 AEA_TELEGLOW, // Teleglow disease
 AEA_NOISE, // Loud noise
 AEA_SCREAM, // Noise & morale penalty
 AEA_DIM, // Darkens the sky slowly
 AEA_FLASH, // Flashbang
 AEA_VOMIT, // User vomits
 AEA_SHADOWS, // Summon shadow creatures

 NUM_AEAS
};

enum art_charge
{
 ARTC_NULL,  // Never recharges!
 ARTC_TIME,  // Very slowly recharges with time
 ARTC_SOLAR, // Recharges in sunlight
 ARTC_PAIN,  // Creates pain to recharge
 ARTC_HP,    // Drains HP to recharge
 NUM_ARTCS
};

enum artifact_natural_shape
{
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

enum artifact_natural_property
{
 ARTPROP_NULL,
 ARTPROP_WRIGGLING, //
 ARTPROP_GLOWING, //
 ARTPROP_HUMMING, //
 ARTPROP_MOVING, //
 ARTPROP_WHISPERING, //
 ARTPROP_BREATHING, //
 ARTPROP_DEAD, //
 ARTPROP_ITCHY, //
 ARTPROP_GLITTERING, //
 ARTPROP_ELECTRIC, //
 ARTPROP_SLIMY, //
 ARTPROP_ENGRAVED, //
 ARTPROP_CRACKLING, //
 ARTPROP_WARM, //
 ARTPROP_RATTLING, //
 ARTPROP_SCALED,
 ARTPROP_FRACTAL,
 ARTPROP_MAX
};

#endif
