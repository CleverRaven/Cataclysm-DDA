#ifndef _ARTIFACTDATA_H_
#define _ARTIFACTDATA_H_

#include <vector>
#include "artifact.h"
#include "itype.h"

extern int passive_effect_cost[NUM_AEPS];

extern int active_effect_cost[NUM_AEAS];


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

#endif
