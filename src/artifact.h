#ifndef _ARTIFACT_H_
#define _ARTIFACT_H_

#include "itype.h"
#include "json.h"

#include <string>
#include <vector>
#include <map>

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

extern int passive_effect_cost[NUM_AEPS];

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

extern int active_effect_cost[NUM_AEAS];

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

struct artifact_shape_datum
{
    std::string name;
    std::string desc;
    int volume_min, volume_max;
    int weight_min, weight_max;
};

extern artifact_shape_datum artifact_shape_data[ARTSHAPE_MAX];

struct artifact_property_datum
{
    std::string name;
    std::string desc;
    art_effect_passive passive_good[4];
    art_effect_passive passive_bad[4];
    art_effect_active active_good[4];
    art_effect_active active_bad[4];
};

extern artifact_property_datum artifact_property_data[ARTPROP_MAX];

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
    std::string m1;
    std::string m2;
    int volume_min, volume_max;
    int weight_min, weight_max;
    artifact_weapon_type base_weapon;
    artifact_weapon_type extra_weapons[3];
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

extern artifact_tool_form_datum artifact_tool_form_data[NUM_ARTTOOLFORMS];

struct artifact_weapon_datum
{
    std::string adjective;
    int volume, weight; // Only applicable if this is an *extra* weapon
    int bash_min, bash_max;
    int cut_min, cut_max;
    int to_hit_min, to_hit_max;
    std::string tag;
};

extern artifact_weapon_datum artifact_weapon_data[NUM_ARTWEAPS];

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
    std::string m1;
    std::string m2;
    int volume, weight;
    int encumb;
    int coverage;
    int thickness;
    int env_resist;
    int warmth;
    int storage;
    int melee_bash, melee_cut, melee_hit;
    unsigned char covers;
    bool plural;
    artifact_armor_mod available_mods[5];
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

extern artifact_armor_form_datum artifact_armor_form_data[NUM_ARTARMFORMS];

/* Armor mods alter the normal values of armor.
 * If the basic armor type has "null" as its second material, and the mod has a
 * material attached, the second material will be changed.
 */
extern artifact_armor_form_datum artifact_armor_mod_data[NUM_ARMORMODS];

#define NUM_ART_ADJS 20
extern std::string artifact_adj[NUM_ART_ADJS];

#define NUM_ART_NOUNS 20
extern std::string artifact_noun[NUM_ART_NOUNS];


/* CLASSES */

class it_artifact_tool : public it_tool, public JsonSerializer, public JsonDeserializer
{
public:
    art_charge charge_type;
    std::vector<art_effect_passive> effects_wielded;
    std::vector<art_effect_active>  effects_activated;
    std::vector<art_effect_passive> effects_carried;

    bool is_artifact() { return true; }

    using JsonSerializer::serialize;
    void serialize(JsonOut &json) const;
    using JsonDeserializer::deserialize;
    void deserialize(JsonObject &jo);
    void deserialize(JsonIn &jsin) {
        JsonObject jo = jsin.get_object();
        deserialize(jo);
    }

    it_artifact_tool(JsonObject &jo) : it_tool() { deserialize(jo); };

    it_artifact_tool() : it_tool() {
        ammo = "NULL";
        price = 0;
        def_charges = 0;
        charges_per_use = 1;
        charge_type = ARTC_NULL;
        turns_per_charge = 0;
        revert_to = "null";
        use = &iuse::artifact;
    };
};

class it_artifact_armor : public it_armor, public JsonSerializer, public JsonDeserializer
{
public:
    std::vector<art_effect_passive> effects_worn;

    bool is_artifact() { return true; }

    using JsonSerializer::serialize;
    void serialize(JsonOut &json) const;
    using JsonDeserializer::deserialize;
    void deserialize(JsonObject &jo);
    void deserialize(JsonIn &jsin) {
        JsonObject jo = jsin.get_object();
        deserialize(jo);
    }

    it_artifact_armor(JsonObject &jo) : it_armor() { deserialize(jo); };
    it_artifact_armor() : it_armor() { price = 0; };
};


/* FUNCTIONS */

typedef std::map<std::string,itype*> itypemap;

void init_artifacts();
itype* new_artifact(itypemap &itypes);
itype* new_natural_artifact(itypemap &itypes, artifact_natural_property prop = ARTPROP_NULL);

// note: needs to be called by main() before MAPBUFFER.load
void load_artifacts(const std::string &filename, itypemap &itypes);
void load_artifacts_from_ifstream(std::ifstream &f, itypemap &itypes);

#endif
