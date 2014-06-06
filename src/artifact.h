#ifndef _ARTIFACT_H_
#define _ARTIFACT_H_

#include "itype.h"
#include "json.h"

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
    AEP_SUPER_CLAIRVOYANCE, // See through walls to a great distance
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

enum art_charge {
    ARTC_NULL,  // Never recharges!
    ARTC_TIME,  // Very slowly recharges with time
    ARTC_SOLAR, // Recharges in sunlight
    ARTC_PAIN,  // Creates pain to recharge
    ARTC_HP,    // Drains HP to recharge
    NUM_ARTCS
};

enum artifact_natural_property {
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

/* CLASSES */

class it_artifact_tool : public it_tool, public JsonSerializer, public JsonDeserializer
{
    public:
        art_charge charge_type;
        std::vector<art_effect_passive> effects_wielded;
        std::vector<art_effect_active>  effects_activated;
        std::vector<art_effect_passive> effects_carried;

        bool is_artifact() {
            return true;
        }

        using JsonSerializer::serialize;
        void serialize(JsonOut &json) const;
        using JsonDeserializer::deserialize;
        void deserialize(JsonObject &jo);
        void deserialize(JsonIn &jsin) {
            JsonObject jo = jsin.get_object();
            deserialize(jo);
        }

        it_artifact_tool();

        it_artifact_tool(JsonObject &jo) : it_tool() {
            use_methods.push_back( &iuse::artifact );
            deserialize(jo);
        };
};

class it_artifact_armor : public it_armor, public JsonSerializer, public JsonDeserializer
{
    public:
        std::vector<art_effect_passive> effects_worn;

        bool is_artifact() {
            return true;
        }

        using JsonSerializer::serialize;
        void serialize(JsonOut &json) const;
        using JsonDeserializer::deserialize;
        void deserialize(JsonObject &jo);
        void deserialize(JsonIn &jsin) {
            JsonObject jo = jsin.get_object();
            deserialize(jo);
        }

        it_artifact_armor();

        it_artifact_armor(JsonObject &jo) : it_armor() {
            deserialize(jo);
        };
};


/* FUNCTIONS */

void init_artifacts();
std::string new_artifact();
std::string new_natural_artifact( artifact_natural_property prop );
std::string architects_cube();

// note: needs to be called by main() before MAPBUFFER.load
void load_artifacts(const std::string &filename);
void load_artifacts_from_ifstream(std::ifstream &f);

#endif
