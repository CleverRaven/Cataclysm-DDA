#include <string>
#include <sstream>
#include "input.h"
#include "game.h"
#include "options.h"
#include "output.h"
#include "crafting.h"
#include "setvector.h"
#include "inventory.h"

void draw_recipe_tabs(WINDOW *w, craft_cat tab);

// This function just defines the recipes used throughout the game.
void game::init_recipes()
{
 int id = -1;
 int tl, cl;

 #define RECIPE(result, category, skill1, skill2, difficulty, time, reversible) \
tl = 0; cl = 0; id++;\
recipes.push_back( new recipe(id, result, category, skill1, skill2, difficulty,\
                              time, reversible) )
 #define TOOL(...)  setvector(recipes[id]->tools[tl],      __VA_ARGS__); tl++
 #define COMP(...)  setvector(recipes[id]->components[cl], __VA_ARGS__); cl++

/**
 * Macro Tool Groups
 * Placeholder for imminent better system, this is already ridiculous
 * Usage:
 * TOOL(TG_KNIVES,NULL);
 */

#define TG_KNIVES \
 itm_knife_steak, -1, itm_knife_combat, -1, itm_knife_butcher, -1, itm_pockknife, -1, itm_xacto, -1, itm_scalpel, -1, itm_machete, -1, itm_broadsword, -1

/* A recipe will not appear in your menu until your level in the primary skill
 * is at least equal to the difficulty.  At that point, your chance of success
 * is still not great; a good 25% improvement over the difficulty is important
 */

// NON-CRAFTABLE BUT CAN BE DISASSEMBLED (set category to CC_NONCRAFT)
RECIPE(itm_knife_steak, CC_NONCRAFT, NULL, NULL, 0, 2000, true);
 COMP(itm_spike, 1, NULL);

RECIPE(itm_lawnmower, CC_NONCRAFT, NULL, NULL, 0, 1000, true);
 TOOL(itm_wrench, -1, itm_toolset, -1, NULL);
 COMP(itm_scrap, 8, NULL);
 COMP(itm_spring, 2, NULL);
 COMP(itm_blade, 2, NULL);
 COMP(itm_1cyl_combustion, 1, NULL);
 COMP(itm_pipe, 3, NULL);

RECIPE(itm_lighter, CC_NONCRAFT, NULL, NULL, 0, 100, true);
 COMP(itm_pilot_light, 1, NULL);

RECIPE(itm_tshirt, CC_NONCRAFT, "tailor", NULL, 0, 500, true);
  COMP(itm_rag, 5, NULL);

RECIPE(itm_tshirt_fit, CC_NONCRAFT, "tailor", NULL, 0, 500, true);
  COMP(itm_rag, 5, NULL);

RECIPE(itm_tank_top, CC_NONCRAFT, "tailor", NULL, 0, 500, true);
  COMP(itm_rag, 5, NULL);

 RECIPE(itm_string_36, CC_NONCRAFT, NULL, NULL, 0, 5000, true);
  TOOL(TG_KNIVES, NULL);
  COMP(itm_string_6, 6, NULL);

 RECIPE(itm_rope_6, CC_NONCRAFT, "tailor", NULL, 0, 5000, true);
  TOOL(TG_KNIVES, NULL);
  COMP(itm_string_36, 6, NULL);

 RECIPE(itm_rope_30, CC_NONCRAFT, "tailor", NULL, 0, 5000, true);
  TOOL(TG_KNIVES, NULL);
  COMP(itm_rope_6, 5, NULL);
// CRAFTABLE

// WEAPONS

 RECIPE(itm_makeshift_machete, CC_WEAPON, NULL, NULL, 0, 5000, true);
  COMP(itm_duct_tape, 50, NULL);
  COMP(itm_blade, 1, NULL);

 RECIPE(itm_makeshift_halberd, CC_WEAPON, NULL, NULL, 0, 5000, true);
  COMP(itm_duct_tape, 100, NULL);
  COMP(itm_blade, 1, NULL);
  COMP(itm_stick, 1, itm_mop, 1, itm_broom, 1, NULL);

 RECIPE(itm_spear_wood, CC_WEAPON, NULL, NULL, 0, 800, false);
  TOOL(itm_hatchet, -1, TG_KNIVES, itm_toolset, -1, NULL);
  COMP(itm_stick, 1, itm_broom, 1, itm_mop, 1, itm_2x4, 1, itm_pool_cue, 1, NULL);

 RECIPE(itm_javelin, CC_WEAPON, "survival", NULL, 1, 5000, false);
  TOOL(itm_hatchet, -1, itm_knife_steak, -1, itm_pockknife, -1, itm_knife_combat, -1,
       itm_knife_butcher, -1, itm_machete, -1, NULL);
  TOOL(itm_fire, -1, NULL);
  COMP(itm_spear_wood, 1, NULL);
  COMP(itm_rag, 1, itm_leather, 1, itm_fur, 1, NULL);
  COMP(itm_plant_fibre, 20, itm_sinew, 20, NULL);

 RECIPE(itm_spear_knife, CC_WEAPON, "stabbing", NULL, 0, 600, true);
  COMP(itm_stick, 1, itm_broom, 1, itm_mop, 1, NULL);
  COMP(itm_spike, 1, NULL);
  COMP(itm_string_6, 6, itm_string_36, 1, NULL);

 RECIPE(itm_longbow, CC_WEAPON, "archery", "survival", 2, 15000, true);
  TOOL(itm_hatchet, -1, TG_KNIVES, itm_toolset, -1, NULL);
  COMP(itm_stick, 1, NULL);
  COMP(itm_string_36, 2, NULL);

 RECIPE(itm_arrow_wood, CC_WEAPON, "archery", "survival", 1, 5000, false);
  TOOL(itm_hatchet, -1, TG_KNIVES, itm_toolset, -1, NULL);
  COMP(itm_stick, 1, itm_broom, 1, itm_mop, 1, itm_2x4, 1, itm_bee_sting, 1,
       NULL);

 RECIPE(itm_nailboard, CC_WEAPON, NULL, NULL, 0, 1000, true);
  TOOL(itm_hatchet, -1, itm_hammer, -1, itm_rock, -1, itm_toolset, -1, NULL);
  COMP(itm_2x4, 1, itm_stick, 1, NULL);
  COMP(itm_nail, 6, NULL);

 RECIPE(itm_nailbat, CC_WEAPON, NULL, NULL, 0, 1000, true);
  TOOL(itm_hatchet, -1, itm_hammer, -1, itm_rock, -1, itm_toolset, -1, NULL);
  COMP(itm_bat, 1, NULL);
  COMP(itm_nail, 6, NULL);

// molotovs use 500ml of flammable liquids
 RECIPE(itm_molotov, CC_WEAPON, NULL, NULL, 0, 500, false);
  COMP(itm_rag, 1, NULL);
  COMP(itm_bottle_glass, 1, itm_flask_glass, 1, NULL);
  COMP(itm_whiskey, 14, itm_vodka, 14, itm_rum, 14, itm_tequila, 14, itm_gin, 14, itm_triple_sec, 14,
       itm_gasoline, 400, NULL);

 RECIPE(itm_pipebomb, CC_WEAPON, "mechanics", NULL, 1, 750, false);
  TOOL(itm_hacksaw, -1, itm_toolset, -1, NULL);
  COMP(itm_pipe, 1, NULL);
  COMP(itm_gasoline, 200, itm_shot_bird, 6, itm_shot_00, 2, itm_shot_slug, 2,
       NULL);
  COMP(itm_string_36, 1, itm_string_6, 1, NULL);

 RECIPE(itm_shotgun_sawn, CC_WEAPON, "gun", NULL, 0, 2000, false);
  TOOL(itm_hacksaw, -1, itm_toolset, -1, NULL);
  COMP(itm_shotgun_d, 1, itm_remington_870, 1, itm_mossberg_500, 1, NULL);

 RECIPE(itm_revolver_shotgun, CC_WEAPON, "gun", "mechanics", 2, 6000, false);
  TOOL(itm_hacksaw, -1, itm_toolset, -1, NULL);
  TOOL(itm_welder, 30, itm_toolset, 3, NULL);
  COMP(itm_shotgun_s, 1, NULL);

 RECIPE(itm_saiga_sawn, CC_WEAPON, "gun", NULL, 0, 2000, false);
  TOOL(itm_hacksaw, -1, itm_toolset, -1, NULL);
  COMP(itm_saiga_12, 1, NULL);

 RECIPE(itm_bolt_wood, CC_WEAPON, "mechanics", "archery", 1, 5000, false);
  TOOL(itm_hatchet, -1, TG_KNIVES, itm_toolset, -1, NULL);
  COMP(itm_stick, 1, itm_broom, 1, itm_mop, 1, itm_2x4, 1, itm_bee_sting, 1,
       NULL);

 RECIPE(itm_crossbow, CC_WEAPON, "mechanics", "archery", 3, 15000, true);
  TOOL(itm_wrench, -1, NULL);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  COMP(itm_2x4, 1, itm_stick, 4, NULL);
  COMP(itm_hose, 1, NULL);

 RECIPE(itm_rifle_22, CC_WEAPON, "mechanics", "gun", 3, 12000, true);
  TOOL(itm_hacksaw, -1, itm_toolset, -1, NULL);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  COMP(itm_pipe, 1, NULL);
  COMP(itm_2x4, 1, NULL);

 RECIPE(itm_rifle_9mm, CC_WEAPON, "mechanics", "gun", 3, 14000, true);
  TOOL(itm_hacksaw, -1, itm_toolset, -1, NULL);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  COMP(itm_pipe, 1, NULL);
  COMP(itm_2x4, 1, NULL);

 RECIPE(itm_smg_9mm, CC_WEAPON, "mechanics", "gun", 5, 18000, true);
  TOOL(itm_hacksaw, -1, itm_toolset, -1, NULL);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  TOOL(itm_hammer, -1, itm_rock, -1, itm_hatchet, -1, NULL);

  COMP(itm_pipe, 1, NULL);
  COMP(itm_2x4, 2, NULL);
  COMP(itm_nail, 4, NULL);

 RECIPE(itm_smg_45, CC_WEAPON, "mechanics", "gun", 5, 20000, true);
  TOOL(itm_hacksaw, -1, itm_toolset, -1, NULL);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  TOOL(itm_hammer, -1, itm_rock, -1, itm_hatchet, -1, NULL);
  COMP(itm_pipe, 1, NULL);
  COMP(itm_2x4, 2, NULL);
  COMP(itm_nail, 4, NULL);

 RECIPE(itm_flamethrower_simple, CC_WEAPON, "mechanics", "gun", 6, 12000, true);
  TOOL(itm_hacksaw, -1, itm_toolset, -1, NULL);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  COMP(itm_pilot_light, 2, NULL);
  COMP(itm_pipe, 1, NULL);
  COMP(itm_hose, 2, NULL);
  COMP(itm_bottle_glass, 4, itm_bottle_plastic, 6, NULL);

 RECIPE(itm_launcher_simple, CC_WEAPON, "mechanics", "launcher", 6, 6000, true);
  TOOL(itm_hacksaw, -1, itm_toolset, -1, NULL);
  COMP(itm_pipe, 1, NULL);
  COMP(itm_2x4, 1, NULL);
  COMP(itm_nail, 1, NULL);

 RECIPE(itm_shot_he, CC_WEAPON, "mechanics", "gun", 4, 2000, false);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  COMP(itm_superglue, 1, NULL);
  COMP(itm_shot_slug, 4, NULL);
  COMP(itm_gasoline, 200, NULL);

 RECIPE(itm_acidbomb, CC_WEAPON, "cooking", NULL, 1, 10000, false);
  TOOL(itm_hotplate, 5, itm_toolset, 1, NULL);
  COMP(itm_bottle_glass, 1, itm_flask_glass, 1, NULL);
  COMP(itm_battery, 500, NULL);

 RECIPE(itm_grenade, CC_WEAPON, "mechanics", NULL, 2, 5000, false);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  COMP(itm_pilot_light, 1, NULL);
  COMP(itm_superglue, 1, itm_string_36, 1, NULL);
  COMP(itm_can_food, 1, itm_can_drink, 1, itm_canister_empty, 1, NULL);
  COMP(itm_nail, 30, itm_bb, 100, NULL);
  COMP(itm_shot_bird, 6, itm_shot_00, 3, itm_shot_slug, 2,
     itm_gasoline, 200, itm_gunpowder, 72,  NULL);

 RECIPE(itm_chainsaw_off, CC_WEAPON, "mechanics", NULL, 4, 20000, true);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  TOOL(itm_hammer, -1, itm_hatchet, -1, NULL);
  TOOL(itm_wrench, -1, itm_toolset, -1, NULL);
  COMP(itm_motor, 1, NULL);
  COMP(itm_chain, 1, NULL);

 RECIPE(itm_smokebomb, CC_WEAPON, "cooking", "mechanics", 3, 7500, false);
  TOOL(itm_screwdriver, -1, itm_wrench, -1, itm_toolset, -1, NULL);
  COMP(itm_water, 1, itm_water_clean, 1, itm_salt_water, 1, NULL);
  COMP(itm_candy, 1, itm_cola, 1, NULL);
  COMP(itm_vitamins, 10, itm_aspirin, 8, NULL);
  COMP(itm_canister_empty, 1, itm_can_food, 1, NULL);
  COMP(itm_superglue, 1, NULL);


 RECIPE(itm_gasbomb, CC_WEAPON, "cooking", "mechanics", 4, 8000, false);
  TOOL(itm_screwdriver, -1, itm_wrench, -1, itm_toolset, -1, NULL);
  COMP(itm_bleach, 2, NULL);
  COMP(itm_ammonia, 2, NULL);
  COMP(itm_canister_empty, 1, itm_can_food, 1, NULL);
  COMP(itm_superglue, 1, NULL);

 RECIPE(itm_nx17, CC_WEAPON, "electronics", "mechanics", 8, 40000, true);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  TOOL(itm_soldering_iron, 6, itm_toolset, 6, NULL);
  COMP(itm_vacutainer, 1, NULL);
  COMP(itm_power_supply, 8, NULL);
  COMP(itm_amplifier, 8, NULL);

 RECIPE(itm_mininuke, CC_WEAPON, "mechanics", "electronics", 10, 40000, true);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  TOOL(itm_wrench, -1, itm_toolset, -1, NULL);
  COMP(itm_can_food, 2, itm_steel_chunk, 2, itm_canister_empty, 1, NULL);
  COMP(itm_plut_cell, 6, NULL);
  COMP(itm_battery, 2, NULL);
  COMP(itm_power_supply, 1, NULL);

  RECIPE(itm_9mm, CC_AMMO, "gun", "mechanics", 2, 25000, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_9mm_casing, 50, NULL);
  COMP(itm_smpistol_primer, 50, NULL);
  COMP(itm_gunpowder, 200, NULL);
  COMP(itm_lead, 200, NULL);

 RECIPE(itm_9mmP, CC_AMMO, "gun", "mechanics", 4, 12500, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_9mm_casing, 25, NULL);
  COMP(itm_smpistol_primer, 25, NULL);
  COMP(itm_gunpowder, 125, NULL);
  COMP(itm_lead, 100, NULL);

 RECIPE(itm_9mmP2, CC_AMMO, "gun", "mechanics", 6, 5000, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_9mm_casing, 10, NULL);
  COMP(itm_smpistol_primer, 10, NULL);
  COMP(itm_gunpowder, 60, NULL);
  COMP(itm_lead, 40, NULL);

 RECIPE(itm_38_special, CC_AMMO, "gun", "mechanics", 2, 25000, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_38_casing, 50, NULL);
  COMP(itm_smpistol_primer, 50, NULL);
  COMP(itm_gunpowder, 250, NULL);
  COMP(itm_lead, 250, NULL);

 RECIPE(itm_38_super, CC_AMMO, "gun", "mechanics", 4, 12500, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_38_casing, 25, NULL);
  COMP(itm_smpistol_primer, 25, NULL);
  COMP(itm_gunpowder, 175, NULL);
  COMP(itm_lead, 125, NULL);

 RECIPE(itm_40sw, CC_AMMO, "gun", "mechanics", 3, 30000, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_40_casing, 50, NULL);
  COMP(itm_smpistol_primer, 50, NULL);
  COMP(itm_gunpowder, 300, NULL);
  COMP(itm_lead, 300, NULL);

 RECIPE(itm_10mm, CC_AMMO, "gun", "mechanics", 5, 25000, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_40_casing, 50, NULL);
  COMP(itm_lgpistol_primer, 50, NULL);
  COMP(itm_gunpowder, 400, NULL);
  COMP(itm_lead, 400, NULL);

 RECIPE(itm_44magnum, CC_AMMO, "gun", "mechanics", 4, 25000, false);
  TOOL(itm_press, -1, NULL);
 TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_44_casing, 50, NULL);
  COMP(itm_lgpistol_primer, 50, NULL);
  COMP(itm_gunpowder, 500, NULL);
  COMP(itm_lead, 500, NULL);

 RECIPE(itm_45_acp, CC_AMMO, "gun", "mechanics", 3, 25000, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_45_casing, 50, NULL);
  COMP(itm_lgpistol_primer, 50, NULL);
  COMP(itm_gunpowder, 500, NULL);
  COMP(itm_lead, 400, NULL);

 RECIPE(itm_45_super, CC_AMMO, "gun", "mechanics", 6, 5000, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_45_casing, 10, NULL);
  COMP(itm_lgpistol_primer, 10, NULL);
  COMP(itm_gunpowder, 120, NULL);
  COMP(itm_lead, 100, NULL);

 RECIPE(itm_57mm, CC_AMMO, "gun", "mechanics", 4, 50000, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_57mm_casing, 100, NULL);
  COMP(itm_smrifle_primer, 100, NULL);
  COMP(itm_gunpowder, 400, NULL);
  COMP(itm_lead, 200, NULL);

 RECIPE(itm_46mm, CC_AMMO, "gun", "mechanics", 4, 50000, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_46mm_casing, 100, NULL);
  COMP(itm_smpistol_primer, 100, NULL);
  COMP(itm_gunpowder, 400, NULL);
  COMP(itm_lead, 200, NULL);

 RECIPE(itm_762_m43, CC_AMMO, "gun", "mechanics", 3, 40000, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_762_casing, 80, NULL);
  COMP(itm_lgrifle_primer, 80, NULL);
  COMP(itm_gunpowder, 560, NULL);
  COMP(itm_lead, 400, NULL);

 RECIPE(itm_762_m87, CC_AMMO, "gun", "mechanics", 5, 40000, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_762_casing, 80, NULL);
  COMP(itm_lgrifle_primer, 80, NULL);
  COMP(itm_gunpowder, 640, NULL);
  COMP(itm_lead, 400, NULL);

 RECIPE(itm_223, CC_AMMO, "gun", "mechanics", 3, 20000, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_223_casing, 40, NULL);
  COMP(itm_smrifle_primer, 40, NULL);
  COMP(itm_gunpowder, 160, NULL);
  COMP(itm_lead, 80, NULL);

 RECIPE(itm_556, CC_AMMO, "gun", "mechanics", 5, 20000, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_223_casing, 40, NULL);
  COMP(itm_smrifle_primer, 40, NULL);
  COMP(itm_gunpowder, 240, NULL);
  COMP(itm_lead, 80, NULL);

 RECIPE(itm_556_incendiary, CC_AMMO, "gun", "mechanics", 6, 15000, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_223_casing, 30, NULL);
  COMP(itm_smrifle_primer, 30, NULL);
  COMP(itm_gunpowder, 180, NULL);
  COMP(itm_incendiary, 60, NULL);

 RECIPE(itm_270, CC_AMMO, "gun", "mechanics", 3, 10000, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_3006_casing, 20, NULL);
  COMP(itm_lgrifle_primer, 20, NULL);
  COMP(itm_gunpowder, 200, NULL);
  COMP(itm_lead, 100, NULL);

 RECIPE(itm_3006, CC_AMMO, "gun", "mechanics", 5, 5000, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_3006_casing, 10, NULL);
  COMP(itm_lgrifle_primer, 10, NULL);
  COMP(itm_gunpowder, 120, NULL);
  COMP(itm_lead, 80, NULL);

 RECIPE(itm_3006_incendiary, CC_AMMO, "gun", "mechanics", 7, 2500, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_3006_casing, 5, NULL);
  COMP(itm_lgrifle_primer, 5, NULL);
  COMP(itm_gunpowder, 60, NULL);
  COMP(itm_incendiary, 40, NULL);

 RECIPE(itm_308, CC_AMMO, "gun", "mechanics", 3, 10000, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_308_casing, 20, NULL);
  COMP(itm_lgrifle_primer, 20, NULL);
  COMP(itm_gunpowder, 160, NULL);
  COMP(itm_lead, 120, NULL);

 RECIPE(itm_762_51, CC_AMMO, "gun", "mechanics", 5, 10000, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_308_casing, 20, NULL);
  COMP(itm_lgrifle_primer, 20, NULL);
  COMP(itm_gunpowder, 200, NULL);
  COMP(itm_lead, 120, NULL);

 RECIPE(itm_762_51_incendiary, CC_AMMO, "gun", "mechanics", 6, 5000, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_308_casing, 10, NULL);
  COMP(itm_lgrifle_primer, 10, NULL);
  COMP(itm_gunpowder, 100, NULL);
  COMP(itm_incendiary, 60, NULL);

 RECIPE(itm_shot_bird, CC_AMMO, "gun", "mechanics", 2, 12500, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_shot_hull, 25, NULL);
  COMP(itm_shotgun_primer, 25, NULL);
  COMP(itm_gunpowder, 300, NULL);
  COMP(itm_lead, 400, NULL);

 RECIPE(itm_shot_00, CC_AMMO, "gun", "mechanics", 3, 12500, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_shot_hull, 25, NULL);
  COMP(itm_shotgun_primer, 25, NULL);
  COMP(itm_gunpowder, 600, NULL);
  COMP(itm_lead, 400, NULL);

 RECIPE(itm_shot_slug, CC_AMMO, "gun", "mechanics", 3, 12500, false);
  TOOL(itm_press, -1, NULL);
  TOOL(itm_fire, -1, itm_toolset, 1, itm_hotplate, 4, itm_press, 2, NULL);
  COMP(itm_shot_hull, 25, NULL);
  COMP(itm_shotgun_primer, 25, NULL);
  COMP(itm_gunpowder, 600, NULL);
  COMP(itm_lead, 400, NULL);
/*
 * We need a some Chemicals which arn't implemented to realistically craft this!
RECIPE(itm_c4, CC_WEAPON, "mechanics", "electronics", 4, 8000);
 TOOL(itm_screwdriver, -1, NULL);
 COMP(itm_can_food, 1, itm_steel_chunk, 1, itm_canister_empty, 1, NULL);
 COMP(itm_battery, 1, NULL);
 COMP(itm_superglue,1,NULL);
 COMP(itm_soldering_iron,1,NULL);
 COMP(itm_power_supply, 1, NULL);
*/

// FOOD

 RECIPE(itm_water_clean, CC_DRINK, "cooking", NULL, 0, 1000, false);
  TOOL(itm_hotplate, 3, itm_toolset, 1, itm_fire, -1, NULL);
  TOOL(itm_pan, -1, itm_pot, -1, itm_rock_pot, -1, NULL);
  COMP(itm_water, 1, NULL);

 RECIPE(itm_meat_cooked, CC_FOOD, "cooking", NULL, 0, 5000, false);
  TOOL(itm_hotplate, 7, itm_toolset, 1, itm_fire, -1, NULL);
  TOOL(itm_pan, -1, itm_pot, -1, itm_rock_pot, -1, itm_spear_wood, -1, NULL);
  COMP(itm_meat, 1, NULL);

 RECIPE(itm_dogfood, CC_FOOD, "cooking", NULL, 4, 10000, false);
  TOOL(itm_hotplate, 6, itm_toolset, 1, itm_fire, -1, NULL);
  TOOL(itm_pot, -1, itm_rock_pot, -1, NULL);
  COMP(itm_meat, 1, NULL);
  COMP(itm_veggy,1, itm_veggy_wild, 1,NULL);
  COMP(itm_water,1, NULL);

 RECIPE(itm_veggy_cooked, CC_FOOD, "cooking", NULL, 0, 4000, false);
  TOOL(itm_hotplate, 5, itm_toolset, 1, itm_fire, -1, NULL);
  TOOL(itm_pan, -1, itm_pot, -1, itm_rock_pot, -1, itm_spear_wood, -1, NULL);
  COMP(itm_veggy, 1, NULL);

  RECIPE(itm_veggy_wild_cooked, CC_FOOD, "cooking", NULL, 0, 4000, false);
  TOOL(itm_hotplate, 5, itm_toolset, 3, itm_fire, -1, NULL);
  TOOL(itm_pan, -1, itm_pot, -1, itm_rock_pot, -1, NULL);
  COMP(itm_veggy_wild, 1, NULL);

 RECIPE(itm_spaghetti_cooked, CC_FOOD, "cooking", NULL, 0, 10000, false);
  TOOL(itm_hotplate, 4, itm_toolset, 1, itm_fire, -1, NULL);
  TOOL(itm_pot, -1, itm_rock_pot, -1, NULL);
  COMP(itm_spaghetti_raw, 1, NULL);
  COMP(itm_water, 1, itm_water_clean, 1, NULL);

 RECIPE(itm_cooked_dinner, CC_FOOD, "cooking", NULL, 0, 5000, false);
  TOOL(itm_hotplate, 3, itm_toolset, 1, itm_fire, -1, NULL);
  COMP(itm_frozen_dinner, 1, NULL);

 RECIPE(itm_macaroni_cooked, CC_FOOD, "cooking", NULL, 1, 10000, false);
  TOOL(itm_hotplate, 4, itm_toolset, 1, itm_fire, -1, NULL);
  TOOL(itm_pot, -1, itm_rock_pot, -1, NULL);
  COMP(itm_macaroni_raw, 1, NULL);
  COMP(itm_water, 1, itm_water_clean, 1, NULL);

 RECIPE(itm_potato_baked, CC_FOOD, "cooking", NULL, 1, 15000, false);
  TOOL(itm_hotplate, 3, itm_toolset, 1, itm_fire, -1, NULL);
  TOOL(itm_pan, -1, itm_pot, -1, itm_rock_pot, -1, NULL);
  COMP(itm_potato_raw, 1, NULL);

 RECIPE(itm_tea, CC_DRINK, "cooking", NULL, 0, 4000, false);
  TOOL(itm_hotplate, 2, itm_toolset, 1, itm_fire, -1, NULL);
  TOOL(itm_pot, -1, itm_rock_pot, -1, NULL);
  COMP(itm_tea_raw, 1, NULL);
  COMP(itm_water, 1, itm_water_clean, 1, NULL);

 RECIPE(itm_coffee, CC_DRINK, "cooking", NULL, 0, 4000, false);
  TOOL(itm_hotplate, 2, itm_toolset, 1, itm_fire, -1, NULL);
  TOOL(itm_pot, -1, itm_rock_pot, -1, NULL);
  COMP(itm_coffee_raw, 1, NULL);
  COMP(itm_water, 1, itm_water_clean, 1, NULL);

 RECIPE(itm_oj, CC_DRINK, "cooking", NULL, 1, 5000, false);
  TOOL(itm_rock, -1, itm_toolset, -1, NULL);
  COMP(itm_orange, 2, NULL);
  COMP(itm_water, 1, itm_water_clean, 1, NULL);

 RECIPE(itm_apple_cider, CC_DRINK, "cooking", NULL, 2, 7000, false);
  TOOL(itm_rock, -1, itm_toolset, 1, NULL);
  COMP(itm_apple, 3, NULL);

 RECIPE(itm_long_island, CC_DRINK, "cooking", NULL, 1, 7000, false);
  COMP(itm_cola, 1, NULL);
  COMP(itm_vodka, 1, NULL);
  COMP(itm_gin, 1, NULL);
  COMP(itm_rum, 1, NULL);
  COMP(itm_tequila, 1, NULL);
  COMP(itm_triple_sec, 1, NULL);

 RECIPE(itm_jerky, CC_FOOD, "cooking", NULL, 3, 30000, false);
  TOOL(itm_hotplate, 10, itm_toolset, 1, itm_fire, -1, NULL);
  COMP(itm_salt_water, 1, itm_salt, 4, NULL);
  COMP(itm_meat, 1, NULL);

 RECIPE(itm_V8, CC_FOOD, "cooking", NULL, 2, 5000, false);
  COMP(itm_tomato, 1, NULL);
  COMP(itm_broccoli, 1, NULL);
  COMP(itm_zucchini, 1, NULL);

 RECIPE(itm_broth, CC_FOOD, "cooking", NULL, 2, 10000, false);
  TOOL(itm_hotplate, 5, itm_toolset, 1, itm_fire, -1, NULL);
  TOOL(itm_pot, -1, itm_rock_pot, -1, NULL);
  COMP(itm_water, 1, itm_water_clean, 1, NULL);
  COMP(itm_broccoli, 1, itm_zucchini, 1, itm_veggy, 1, itm_veggy_wild, 1, NULL);

 RECIPE(itm_soup_veggy, CC_FOOD, "cooking", NULL, 2, 10000, false);
  TOOL(itm_hotplate, 5, itm_toolset, 1, itm_fire, -1, NULL);
  TOOL(itm_pot, -1, itm_rock_pot, -1, NULL);
  COMP(itm_broth, 2, NULL);
  COMP(itm_macaroni_raw, 1, itm_potato_raw, 1, NULL);
  COMP(itm_tomato, 2, itm_broccoli, 2, itm_zucchini, 2, itm_veggy, 2, itm_veggy_wild, 2, NULL);

 RECIPE(itm_soup_meat, CC_FOOD, "cooking", NULL, 2, 10000, false);
  TOOL(itm_hotplate, 5, itm_toolset, 1, itm_fire, -1, NULL);
  TOOL(itm_pot, -1, itm_rock_pot, -1, NULL);
  COMP(itm_broth, 2, NULL);
  COMP(itm_macaroni_raw, 1, itm_potato_raw, 1, NULL);
  COMP(itm_meat, 2, NULL);

 RECIPE(itm_bread, CC_FOOD, "cooking", NULL, 4, 20000, false);
  TOOL(itm_hotplate, 8, itm_toolset, 1, itm_fire, -1, NULL);
  TOOL(itm_pot, -1, itm_rock_pot, -1, NULL);
  COMP(itm_flour, 3, NULL);
  COMP(itm_water, 2, itm_water_clean, 2, NULL);

 RECIPE(itm_pie, CC_FOOD, "cooking", NULL, 3, 25000, false);
  TOOL(itm_hotplate, 6, itm_toolset, 1, itm_fire, -1, NULL);
  TOOL(itm_pan, -1, NULL);
  COMP(itm_flour, 2, NULL);
  COMP(itm_strawberries, 2, itm_apple, 2, itm_blueberries, 2, NULL);
  COMP(itm_sugar, 2, NULL);
  COMP(itm_water, 1, itm_water_clean, 1, NULL);

 RECIPE(itm_pizza, CC_FOOD, "cooking", NULL, 3, 20000, false);
  TOOL(itm_hotplate, 8, itm_toolset, 1, itm_fire, -1, NULL);
  TOOL(itm_pan, -1, NULL);
  COMP(itm_flour, 2, NULL);
  COMP(itm_veggy, 1, itm_veggy_wild, 1, itm_tomato, 2, itm_broccoli, 1, NULL);
  COMP(itm_sauce_pesto, 1, itm_sauce_red, 1, NULL);
  COMP(itm_water, 1, itm_water_clean, 1, NULL);

 RECIPE(itm_meth, CC_CHEM, "cooking", NULL, 5, 20000, false);
  TOOL(itm_hotplate, 15, itm_toolset, 1, itm_fire, -1, NULL);
  TOOL(itm_bottle_glass, -1, itm_hose, -1, NULL);
  COMP(itm_dayquil, 2, itm_royal_jelly, 1, NULL);
  COMP(itm_aspirin, 40, NULL);
  COMP(itm_caffeine, 20, itm_adderall, 5, itm_energy_drink, 2, NULL);

 RECIPE(itm_crack,        CC_CHEM, "cooking", NULL,     4, 30000,false);
  TOOL(itm_pot, -1, itm_rock_pot, -1, NULL);
  TOOL(itm_fire, -1, itm_hotplate, 8, itm_toolset, 1, NULL);
  COMP(itm_water, 1, itm_water_clean, 1, NULL);
  COMP(itm_coke, 12, NULL);
  COMP(itm_ammonia, 1, NULL);

 RECIPE(itm_poppy_sleep,  CC_CHEM, "cooking", "survival", 2, 5000, false);
  TOOL(itm_pot, -1, itm_rock_pot, -1, itm_rock, -1, NULL);
  TOOL(itm_fire, -1, NULL);
  COMP(itm_poppy_bud, 2, NULL);
  COMP(itm_poppy_flower, 1, NULL);

 RECIPE(itm_poppy_pain,  CC_CHEM, "cooking", "survival", 2, 5000, false);
  TOOL(itm_pot, -1, itm_rock_pot, -1, itm_rock, -1, NULL);
  TOOL(itm_fire, -1, NULL);
  COMP(itm_poppy_bud, 2, NULL);
  COMP(itm_poppy_flower, 2, NULL);

 RECIPE(itm_royal_jelly, CC_CHEM, "cooking", NULL, 5, 5000, false);
  COMP(itm_honeycomb, 1, NULL);
  COMP(itm_bleach, 2, itm_purifier, 1, NULL);

 RECIPE(itm_heroin, CC_CHEM, "cooking", NULL, 6, 2000, false);
  TOOL(itm_hotplate, 3, itm_toolset, 1, itm_fire, -1, NULL);
  TOOL(itm_pan, -1, itm_pot, -1, itm_rock_pot, -1, NULL);
  COMP(itm_salt_water, 1, itm_salt, 4, NULL);
  COMP(itm_oxycodone, 40, NULL);

 RECIPE(itm_mutagen, CC_CHEM, "cooking", "firstaid", 8, 10000, false);
  TOOL(itm_hotplate, 25, itm_toolset, 2, itm_fire, -1, NULL);
  COMP(itm_meat_tainted, 3, itm_veggy_tainted, 5, itm_fetus, 1, itm_arm, 2,
       itm_leg, 2, NULL);
  COMP(itm_bleach, 2, NULL);
  COMP(itm_ammonia, 1, NULL);

 RECIPE(itm_purifier, CC_CHEM, "cooking", "firstaid", 9, 10000, false);
  TOOL(itm_hotplate, 25, itm_toolset, 2, itm_fire, -1, NULL);
  COMP(itm_royal_jelly, 4, itm_mutagen, 2, NULL);
  COMP(itm_bleach, 3, NULL);
  COMP(itm_ammonia, 2, NULL);

// ELECTRONICS

 RECIPE(itm_antenna, CC_ELECTRONIC, NULL, NULL, 0, 3000, false);
  TOOL(itm_hacksaw, -1, itm_toolset, -1, NULL);
  COMP(itm_knife_butter, 2, NULL);

 RECIPE(itm_amplifier, CC_ELECTRONIC, "electronics", NULL, 1, 4000, false);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  COMP(itm_transponder, 2, NULL);

 RECIPE(itm_power_supply, CC_ELECTRONIC, "electronics", NULL, 1, 6500, false);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  TOOL(itm_soldering_iron, 3, itm_toolset, 3, NULL);
  COMP(itm_amplifier, 2, NULL);
  COMP(itm_cable, 20, NULL);

 RECIPE(itm_receiver, CC_ELECTRONIC, "electronics", NULL, 2, 12000, true);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  TOOL(itm_soldering_iron, 4, itm_toolset, 4, NULL);
  COMP(itm_amplifier, 2, NULL);
  COMP(itm_cable, 10, NULL);

 RECIPE(itm_transponder, CC_ELECTRONIC, "electronics", NULL, 2, 14000, true);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  TOOL(itm_soldering_iron, 7, itm_toolset, 7, NULL);
  COMP(itm_receiver, 3, NULL);
  COMP(itm_cable, 5, NULL);

 RECIPE(itm_flashlight, CC_ELECTRONIC, "electronics", NULL, 1, 10000, true);
  COMP(itm_amplifier, 1, NULL);
  COMP(itm_scrap, 4, itm_can_drink, 1, itm_bottle_glass, 1, itm_bottle_plastic, 1, NULL);
  COMP(itm_cable, 10, NULL);

 RECIPE(itm_soldering_iron, CC_ELECTRONIC, "electronics", NULL, 1, 20000, true);
  COMP(itm_antenna, 1, itm_screwdriver, 1, itm_xacto, 1, itm_knife_butter, 1,
       NULL);
  COMP(itm_power_supply, 1, NULL);
  COMP(itm_scrap, 2, NULL);

 RECIPE(itm_battery, CC_ELECTRONIC, "electronics", "mechanics", 2, 5000, false);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  COMP(itm_ammonia, 1, itm_lemon, 1, NULL);
  COMP(itm_steel_chunk, 1, itm_knife_butter, 1, itm_knife_steak, 1,
     itm_bolt_steel, 1, itm_scrap, 1, NULL);
  COMP(itm_can_drink, 1, itm_can_food, 1, NULL);

 RECIPE(itm_coilgun, CC_WEAPON, "electronics", NULL, 3, 25000, true);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  TOOL(itm_soldering_iron, 10, itm_toolset, 10, NULL);
  COMP(itm_pipe, 1, NULL);
  COMP(itm_power_supply, 1, NULL);
  COMP(itm_amplifier, 1, NULL);
  COMP(itm_scrap, 6, NULL);
  COMP(itm_cable, 20, NULL);

 RECIPE(itm_radio, CC_ELECTRONIC, "electronics", NULL, 2, 25000, true);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  TOOL(itm_soldering_iron, 10, itm_toolset, 10, NULL);
  COMP(itm_receiver, 1, NULL);
  COMP(itm_antenna, 1, NULL);
  COMP(itm_scrap, 5, NULL);
  COMP(itm_cable, 7, NULL);

 RECIPE(itm_water_purifier, CC_ELECTRONIC, "mechanics","electronics",3,25000, true);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  COMP(itm_element, 2, NULL);
  COMP(itm_bottle_glass, 2, itm_bottle_plastic, 5, NULL);
  COMP(itm_hose, 1, NULL);
  COMP(itm_scrap, 3, NULL);
  COMP(itm_cable, 5, NULL);

 RECIPE(itm_hotplate, CC_ELECTRONIC, "electronics", NULL, 3, 30000, true);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  COMP(itm_element, 1, NULL);
  COMP(itm_amplifier, 1, NULL);
  COMP(itm_scrap, 2, itm_pan, 1, itm_pot, 1, itm_knife_butcher, 2, itm_knife_steak, 6,
     itm_knife_butter, 6, itm_muffler, 1, NULL);
  COMP(itm_cable, 10, NULL);

 RECIPE(itm_tazer, CC_ELECTRONIC, "electronics", NULL, 3, 25000, true);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  TOOL(itm_soldering_iron, 10, itm_toolset, 10, NULL);
  COMP(itm_amplifier, 1, NULL);
  COMP(itm_power_supply, 1, NULL);
  COMP(itm_scrap, 2, NULL);

 RECIPE(itm_two_way_radio, CC_ELECTRONIC, "electronics", NULL, 4, 30000, true);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  TOOL(itm_soldering_iron, 14, itm_toolset, 14, NULL);
  COMP(itm_amplifier, 1, NULL);
  COMP(itm_transponder, 1, NULL);
  COMP(itm_receiver, 1, NULL);
  COMP(itm_antenna, 1, NULL);
  COMP(itm_scrap, 5, NULL);
  COMP(itm_cable, 10, NULL);

 RECIPE(itm_electrohack, CC_ELECTRONIC, "electronics", "computer", 4, 35000, true);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  TOOL(itm_soldering_iron, 10, itm_toolset, 10, NULL);
  COMP(itm_processor, 1, NULL);
  COMP(itm_RAM, 1, NULL);
  COMP(itm_scrap, 4, NULL);
  COMP(itm_cable, 10, NULL);

 RECIPE(itm_EMPbomb, CC_ELECTRONIC, "electronics", NULL, 4, 32000, false);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  TOOL(itm_soldering_iron, 6, itm_toolset, 6, NULL);
  COMP(itm_superglue, 1, itm_string_36, 1, NULL);
  COMP(itm_scrap, 3, itm_can_food, 1, itm_can_drink, 1, itm_canister_empty, 1, NULL);
  COMP(itm_power_supply, 1, itm_amplifier, 1, NULL);
  COMP(itm_cable, 5, NULL);

 RECIPE(itm_mp3, CC_ELECTRONIC, "electronics", "computer", 5, 40000, true);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  TOOL(itm_soldering_iron, 5, itm_toolset, 5, NULL);
  COMP(itm_superglue, 1, NULL);
  COMP(itm_antenna, 1, NULL);
  COMP(itm_amplifier, 1, NULL);
  COMP(itm_cable, 2, NULL);

 RECIPE(itm_geiger_off, CC_ELECTRONIC, "electronics", NULL, 5, 35000, true);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  TOOL(itm_soldering_iron, 14, itm_toolset, 14, NULL);
  COMP(itm_power_supply, 1, NULL);
  COMP(itm_amplifier, 2, NULL);
  COMP(itm_scrap, 6, NULL);
  COMP(itm_cable, 10, NULL);

 RECIPE(itm_UPS_off, CC_ELECTRONIC, "electronics", NULL, 5, 45000, true);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  TOOL(itm_soldering_iron, 24, itm_toolset, 24, NULL);
  COMP(itm_power_supply, 4, NULL);
  COMP(itm_amplifier, 3, NULL);
  COMP(itm_scrap, 4, NULL);
  COMP(itm_cable, 10, NULL);

 RECIPE(itm_bionics_battery, CC_ELECTRONIC, "electronics", NULL, 6, 50000, true);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  TOOL(itm_soldering_iron, 20, itm_toolset, 20, NULL);
  COMP(itm_power_supply, 6, itm_UPS_off, 1, NULL);
  COMP(itm_amplifier, 4, NULL);
  COMP(itm_plut_cell, 1, NULL);

 RECIPE(itm_teleporter, CC_ELECTRONIC, "electronics", NULL, 8, 50000, true);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  TOOL(itm_wrench, -1, itm_toolset, -1, NULL);
  TOOL(itm_soldering_iron, 16, itm_toolset, 16, NULL);
  COMP(itm_power_supply, 3, itm_plut_cell, 5, NULL);
  COMP(itm_amplifier, 3, NULL);
  COMP(itm_transponder, 3, NULL);
  COMP(itm_scrap, 10, NULL);
  COMP(itm_cable, 20, NULL);

// ARMOR

 RECIPE(itm_mocassins, CC_ARMOR, "tailor", NULL, 1, 30000, false);
  TOOL(itm_needle_bone, 5, itm_sewing_kit,  5, NULL);
  COMP(itm_fur, 2, NULL);

 RECIPE(itm_boots_fit, CC_ARMOR, "tailor", NULL, 2, 35000, false);
  TOOL(itm_needle_bone, 5, itm_sewing_kit, 10, NULL);
  COMP(itm_leather, 7, NULL);

 RECIPE(itm_jeans_fit, CC_ARMOR, "tailor", NULL, 2, 45000, false);
  TOOL(itm_needle_bone, 10, itm_sewing_kit, 10, NULL);
  COMP(itm_rag, 6, NULL);

 RECIPE(itm_pants_cargo_fit, CC_ARMOR, "tailor", NULL, 3, 48000, false);
  TOOL(itm_needle_bone, 16, itm_sewing_kit, 16, NULL);
  COMP(itm_rag, 8, NULL);

 RECIPE(itm_pants_leather, CC_ARMOR, "tailor", NULL, 4, 50000, false);
  TOOL(itm_needle_bone, 10, itm_sewing_kit, 10, NULL);
  COMP(itm_leather, 10, NULL);

 RECIPE(itm_tank_top, CC_ARMOR, "tailor", NULL, 2, 38000, true);
  TOOL(itm_needle_bone, 4, itm_sewing_kit, 4, NULL);
  COMP(itm_rag, 4, NULL);

RECIPE(itm_tshirt_fit, CC_ARMOR, "tailor", NULL, 2, 38000, true);
  TOOL(itm_needle_bone, 4, itm_sewing_kit, 4, NULL);
  COMP(itm_rag, 5, NULL);

 RECIPE(itm_hoodie_fit, CC_ARMOR, "tailor", NULL, 3, 40000, false);
  TOOL(itm_needle_bone, 14, itm_sewing_kit, 14, NULL);
  COMP(itm_rag, 12, NULL);

 RECIPE(itm_trenchcoat_fit, CC_ARMOR, "tailor", NULL, 3, 42000, false);
  TOOL(itm_needle_bone, 24, itm_sewing_kit, 24, NULL);
  COMP(itm_rag, 11, NULL);

 RECIPE(itm_coat_fur, CC_ARMOR, "tailor", NULL, 4, 100000, false);
  TOOL(itm_needle_bone, 20, itm_sewing_kit, 20, NULL);
  COMP(itm_fur, 10, NULL);

 RECIPE(itm_jacket_leather_fit, CC_ARMOR, "tailor", NULL, 5, 150000, false);
  TOOL(itm_needle_bone, 30, itm_sewing_kit, 30, NULL);
  COMP(itm_leather, 16, NULL);

 RECIPE(itm_gloves_light, CC_ARMOR, "tailor", NULL, 1, 10000, false);
  TOOL(itm_needle_bone, 2, itm_sewing_kit, 2, NULL);
  COMP(itm_rag, 2, NULL);

 RECIPE(itm_gloves_fingerless, CC_ARMOR, "tailor", NULL, 0, 16000, false);
  TOOL(itm_scissors, -1, TG_KNIVES, NULL);
  COMP(itm_gloves_leather, 1, NULL);

 RECIPE(itm_gloves_leather, CC_ARMOR, "tailor", NULL, 2, 16000, false);
  TOOL(itm_needle_bone, 6, itm_sewing_kit, 6, NULL);
  COMP(itm_leather, 2, NULL);

 RECIPE(itm_mask_filter, CC_ARMOR, "mechanics", "tailor", 1, 5000, true);
  COMP(/*itm_filter, 1, */itm_bag_plastic, 2, itm_bottle_plastic, 1, NULL);
  COMP(itm_rag, 2, itm_muffler, 1, itm_bandana, 2, itm_wrapper, 4, NULL);

 RECIPE(itm_mask_gas, CC_ARMOR, "tailor", NULL, 3, 20000, true);
  TOOL(itm_wrench, -1, itm_toolset, -1, NULL);
  COMP(itm_goggles_ski, 1, itm_goggles_swim, 2, NULL);
  COMP(/*itm_filter, 3, */itm_mask_filter, 3, itm_muffler, 1, NULL);
  COMP(itm_hose, 1, NULL);

 RECIPE(itm_glasses_safety, CC_ARMOR, "tailor", NULL, 1, 8000, false);
  TOOL(itm_scissors, -1, TG_KNIVES, itm_toolset, -1, NULL);
  COMP(itm_string_36, 1, itm_string_6, 2, NULL);
  COMP(itm_bottle_plastic, 1, NULL);

 RECIPE(itm_goggles_nv, CC_ARMOR, "electronics", "tailor", 5, 40000, true);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  COMP(itm_goggles_ski, 1, itm_goggles_welding, 1, itm_mask_gas, 1, NULL);
  COMP(itm_power_supply, 1, NULL);
  COMP(itm_amplifier, 3, NULL);
  COMP(itm_scrap, 5, NULL);

 RECIPE(itm_hat_fur, CC_ARMOR, "tailor", NULL, 2, 40000, false);
  TOOL(itm_needle_bone, 8, itm_sewing_kit, 8, NULL);
  COMP(itm_fur, 3, NULL);

 RECIPE(itm_armguard_metal, CC_ARMOR, "tailor", NULL, 4,  30000, false);
  TOOL(itm_hammer, -1, itm_toolset, -1, NULL);
  COMP(itm_string_36, 1, itm_string_6, 4, NULL);
  COMP(itm_steel_chunk, 2, NULL);

 RECIPE(itm_armguard_chitin, CC_ARMOR, "tailor", NULL, 3,  30000, false);
  COMP(itm_string_36, 1, itm_string_6, 4, NULL);
  COMP(itm_chitin_piece, 6, NULL);

 RECIPE(itm_boots_chitin, CC_ARMOR, "tailor", NULL, 3,  30000, false);
  COMP(itm_string_36, 1, itm_string_6, 4, NULL);
  COMP(itm_chitin_piece, 4, NULL);
  COMP(itm_leather, 2, itm_fur, 2, itm_rag, 2, NULL);

 RECIPE(itm_gauntlets_chitin, CC_ARMOR, "tailor", NULL, 3,  30000, false);
  COMP(itm_string_36, 1, itm_string_6, 4, NULL);
  COMP(itm_chitin_piece, 4, NULL);

 RECIPE(itm_helmet_chitin, CC_ARMOR, "tailor", NULL, 6,  60000, false);
  COMP(itm_string_36, 1, itm_string_6, 5, NULL);
  COMP(itm_chitin_piece, 5, NULL);

 RECIPE(itm_armor_chitin, CC_ARMOR, "tailor", NULL,  7, 100000, false);
  COMP(itm_string_36, 2, itm_string_6, 12, NULL);
  COMP(itm_chitin_piece, 15, NULL);

 RECIPE(itm_backpack, CC_ARMOR, "tailor", NULL, 3, 50000, false);
  TOOL(itm_needle_bone, 20, itm_sewing_kit, 20, NULL);
  COMP(itm_rag, 20, itm_fur, 16, itm_leather, 12, NULL);


// SURVIVAL

 RECIPE(itm_primitive_hammer, CC_MISC, "survival", "construction", 0, 5000, false);
  TOOL(itm_rock, -1, itm_hammer, -1, itm_toolset, -1, NULL);
  COMP(itm_stick, 1, NULL);
  COMP(itm_rock, 1, NULL);
  COMP(itm_string_6, 2, itm_sinew, 40, itm_plant_fibre, 40, NULL);

 RECIPE(itm_needle_bone, CC_MISC, "survival", NULL, 3, 20000, false);
  TOOL(TG_KNIVES, itm_toolset, -1, NULL);
  COMP(itm_bone, 1, NULL);

 RECIPE(itm_ragpouch, CC_ARMOR, "tailor",  NULL, 0, 10000, false);
  TOOL(itm_needle_bone, 20, itm_sewing_kit, 20,  NULL);
  COMP(itm_rag, 6, NULL);
  COMP(itm_string_36, 1, itm_string_6, 6, itm_sinew, 20, itm_plant_fibre, 20, NULL);

 RECIPE(itm_leather_pouch, CC_ARMOR, "tailor",  "survival", 2, 10000, false);
  TOOL(itm_needle_bone, 20, itm_sewing_kit, 20, NULL);
  COMP(itm_leather, 6, NULL);
  COMP(itm_string_36, 1, itm_string_6, 6, itm_sinew, 20, itm_plant_fibre, 20, NULL);

 RECIPE(itm_rock_pot, CC_MISC, "survival", "cooking", 2, 20000, false);
  TOOL(itm_hammer, -1, itm_primitive_hammer, -1, itm_toolset, -1, NULL);
  COMP(itm_rock, 3, NULL);
  COMP(itm_sinew, 80, itm_plant_fibre, 80, itm_string_36, 1, NULL);

 RECIPE(itm_primitive_shovel, CC_MISC, "survival", "construction", 2, 60000, false);
  TOOL(itm_primitive_hammer, -1, itm_hammer, -1, itm_toolset, -1, NULL);
  COMP(itm_stick, 1, NULL);
  COMP(itm_rock, 1, NULL);
  COMP(itm_string_6, 2, itm_sinew, 40, itm_plant_fibre, 40, NULL);

 RECIPE(itm_primitive_axe, CC_MISC, "survival", "construction", 3, 60000, false);
  TOOL(itm_primitive_hammer, -1, itm_hammer, -1, itm_toolset, -1, NULL);
  COMP(itm_stick, 1, NULL);
  COMP(itm_rock, 1, NULL);
  COMP(itm_string_6, 2, itm_sinew, 40, itm_plant_fibre, 40, NULL);

 RECIPE(itm_waterskin, CC_MISC, "tailor", "survival", 2, 30000, false);
  TOOL(itm_sewing_kit, 60, itm_needle_bone, 60, NULL);
  COMP(itm_sinew, 40, itm_plant_fibre, 40, itm_string_36, 1, NULL);
  COMP(itm_leather, 6, itm_fur, 6, NULL);
// MISC


 RECIPE(itm_rag, CC_MISC, NULL, NULL, 0, 3000, false);
  TOOL(itm_fire, -1, itm_hotplate, 3, itm_toolset, 1, NULL);
  COMP(itm_water, 1, itm_water_clean, 1, NULL);
  COMP(itm_rag_bloody, 1, NULL);

 RECIPE(itm_sheet, CC_MISC, NULL, NULL, 0, 10000, false);
  TOOL(itm_sewing_kit, 50, NULL);
  COMP(itm_rag, 20, NULL);

 RECIPE(itm_vehicle_controls, CC_MISC, "mechanics", NULL, 3, 30000, true);
  TOOL(itm_wrench, -1, itm_toolset, -1, NULL);
  TOOL(itm_hammer, -1, itm_toolset, -1, NULL);
  TOOL(itm_welder, 50, itm_toolset, 5, NULL);
  TOOL(itm_hacksaw, -1, itm_toolset, -1, NULL);
  COMP(itm_pipe, 10, NULL);
  COMP(itm_steel_chunk, 12, NULL);
  COMP(itm_wire, 3, NULL);


 RECIPE(itm_thread, CC_MISC, "tailor", NULL, 1, 3000, false);
  COMP(itm_string_6, 1, NULL);

 RECIPE(itm_string_6, CC_MISC, NULL, NULL, 0, 5000, true);
  COMP(itm_thread, 50, NULL);

 RECIPE(itm_string_36, CC_MISC, NULL, NULL, 0, 5000, true);
  COMP(itm_string_6, 6, NULL);

 RECIPE(itm_rope_6, CC_MISC, "tailor", NULL, 0, 5000, true);
  COMP(itm_string_36, 6, NULL);

 RECIPE(itm_rope_30, CC_MISC, "tailor", NULL, 0, 5000, true);
  COMP(itm_rope_6, 5, NULL);

 RECIPE(itm_torch,        CC_MISC, NULL,    NULL,     0, 2000, false);
  COMP(itm_stick, 1, itm_2x4, 1, itm_splinter, 1, itm_pool_cue, 1, itm_torch_done, 1, NULL);
  COMP(itm_gasoline, 200, itm_vodka, 7, itm_rum, 7, itm_whiskey, 7, itm_tequila, 7, itm_gin, 7, itm_triple_sec, 7, NULL);
  COMP(itm_rag, 1, NULL);

 RECIPE(itm_candle,       CC_MISC, NULL,    NULL,     0, 5000, false);
  TOOL(itm_lighter, 5, itm_fire, -1, itm_toolset, 1, NULL);
  COMP(itm_can_food, -1, NULL);
  COMP(itm_wax, 2, NULL);
  COMP(itm_string_6, 1, NULL);

 RECIPE(itm_spike,     CC_MISC, NULL, NULL, 0, 3000, false);
  TOOL(itm_hammer, -1, itm_toolset, -1, NULL);
  COMP(itm_knife_combat, 1, itm_steel_chunk, 3, itm_scrap, 9, NULL);

 RECIPE(itm_blade,     CC_MISC, NULL, NULL, 0, 3000, false);
  TOOL(itm_hammer, -1, itm_toolset, -1, NULL);
  COMP(itm_broadsword, 1, itm_machete, 1, itm_pike, 1, NULL);

 RECIPE(itm_superglue, CC_MISC, "cooking", NULL, 2, 12000, false);
  TOOL(itm_hotplate, 5, itm_toolset, 1, itm_fire, -1, NULL);
  COMP(itm_water, 1, itm_water_clean, 1, NULL);
  COMP(itm_bleach, 1, itm_ant_egg, 1, NULL);

 RECIPE(itm_steel_lump, CC_MISC, "mechanics", NULL, 0, 5000, true);
  TOOL(itm_welder, 20, itm_toolset, 1, NULL);
  COMP(itm_steel_chunk, 4, NULL);

 RECIPE(itm_2x4, CC_MISC, NULL, NULL, 0, 8000, false);
  TOOL(itm_saw, -1, NULL);
  COMP(itm_stick, 1, NULL);

 RECIPE(itm_frame, CC_MISC, "mechanics", NULL, 1, 8000, true);
  TOOL(itm_welder, 50, itm_toolset, 2, NULL);
  COMP(itm_steel_lump, 3, NULL);

 RECIPE(itm_steel_plate, CC_MISC, "mechanics", NULL,4, 12000, true);
  TOOL(itm_welder, 100, itm_toolset, 4, NULL);
  COMP(itm_steel_lump, 8, NULL);

 RECIPE(itm_spiked_plate, CC_MISC, "mechanics", NULL, 4, 12000, true);
  TOOL(itm_welder, 120, itm_toolset, 5, NULL);
  COMP(itm_steel_lump, 8, NULL);
  COMP(itm_steel_chunk, 4, itm_scrap, 8, NULL);

 RECIPE(itm_hard_plate, CC_MISC, "mechanics", NULL, 4, 12000, true);
  TOOL(itm_welder, 300, itm_toolset, 12, NULL);
  COMP(itm_steel_lump, 24, NULL);

 RECIPE(itm_crowbar, CC_MISC, "mechanics", NULL, 1, 1000, false);
  TOOL(itm_hatchet, -1, itm_hammer, -1, itm_rock, -1, itm_toolset, -1, NULL);
  COMP(itm_pipe, 1, NULL);

 RECIPE(itm_bayonet, CC_MISC, "gun", NULL, 1, 500, true);
  COMP(itm_spike, 1, NULL);
  COMP(itm_string_36, 1, NULL);

 RECIPE(itm_tripwire, CC_MISC, "traps", NULL, 1, 500, false);
  COMP(itm_string_36, 1, NULL);
  COMP(itm_superglue, 1, NULL);

 RECIPE(itm_board_trap, CC_MISC, "traps", NULL, 2, 2500, true);
  TOOL(itm_hatchet, -1, itm_hammer, -1, itm_rock, -1, itm_toolset, -1, NULL);
  COMP(itm_2x4, 3, NULL);
  COMP(itm_nail, 20, NULL);

 RECIPE(itm_beartrap, CC_MISC, "mechanics", "traps", 2, 3000, true);
  TOOL(itm_wrench, -1, itm_toolset, -1, NULL);
  COMP(itm_scrap, 3, NULL);
  COMP(itm_spring, 1, NULL);

 RECIPE(itm_crossbow_trap, CC_MISC, "mechanics", "traps", 3, 4500, true);
  COMP(itm_crossbow, 1, NULL);
  COMP(itm_bolt_steel, 1, itm_bolt_wood, 4, NULL);
  COMP(itm_string_6, 2, itm_string_36, 1, NULL);

 RECIPE(itm_shotgun_trap, CC_MISC, "mechanics", "traps", 3, 5000, true);
  COMP(itm_shotgun_sawn, 1, NULL);
  COMP(itm_shot_00, 2, NULL);
  COMP(itm_string_36, 1, itm_string_6, 2, NULL);

 RECIPE(itm_blade_trap, CC_MISC, "mechanics", "traps", 4, 8000, true);
  TOOL(itm_wrench, -1, itm_toolset, -1, NULL);
  COMP(itm_motor, 1, NULL);
  COMP(itm_blade, 1, NULL);
  COMP(itm_string_36, 1, NULL);

RECIPE(itm_boobytrap, CC_MISC, "mechanics", "traps",3,5000, false);
  COMP(itm_grenade,1,NULL);
  COMP(itm_string_6,1,NULL);
  COMP(itm_can_food,1,NULL);

 RECIPE(itm_landmine, CC_MISC, "traps", "mechanics", 5, 10000, false);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  COMP(itm_superglue, 1, NULL);
  COMP(itm_can_food, 1, itm_steel_chunk, 1, itm_canister_empty, 1, itm_scrap, 4, NULL);
  COMP(itm_nail, 100, itm_bb, 200, NULL);
  COMP(itm_shot_bird, 30, itm_shot_00, 15, itm_shot_slug, 12, itm_gasoline, 600,
     itm_grenade, 1, itm_gunpowder, 72, NULL);

 RECIPE(itm_bandages, CC_MISC, "firstaid", NULL, 1, 500, false);
  COMP(itm_rag, 3, NULL);
  COMP(itm_superglue, 1, itm_duct_tape, 5, NULL);
  COMP(itm_vodka, 7, itm_rum, 7, itm_whiskey, 7, itm_tequila, 7, itm_gin, 7, itm_triple_sec, 7, NULL);

 RECIPE(itm_silencer, CC_MISC, "mechanics", NULL, 1, 650, false);
  TOOL(itm_hacksaw, -1, itm_toolset, -1, NULL);
  COMP(itm_muffler, 1, itm_rag, 4, NULL);
  COMP(itm_pipe, 1, NULL);

 RECIPE(itm_pheromone, CC_MISC, "cooking", NULL, 3, 1200, false);
  TOOL(itm_hotplate, 18, itm_toolset, 9, itm_fire, -1, NULL);
  COMP(itm_meat_tainted, 1, NULL);
  COMP(itm_ammonia, 1, NULL);

 RECIPE(itm_laser_pack, CC_MISC, "electronics", NULL, 5, 10000, true);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  COMP(itm_superglue, 1, NULL);
  COMP(itm_plut_cell, 1, NULL);

 RECIPE(itm_bot_manhack, CC_MISC, "electronics", "computer", 6, 8000, true);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  TOOL(itm_soldering_iron, 10, itm_toolset, 10, NULL);
  COMP(itm_spike, 2, NULL);
  COMP(itm_processor, 1, NULL);
  COMP(itm_RAM, 1, NULL);
  COMP(itm_power_supply, 1, NULL);
//  COMP(itm_battery, 400, itm_plut_cell, 1, NULL);
//  COMP(itm_scrap, 15, NULL);

 RECIPE(itm_bot_turret, CC_MISC, "electronics", "computer", 7, 9000, true);
  TOOL(itm_screwdriver, -1, itm_toolset, -1, NULL);
  TOOL(itm_soldering_iron, 14, itm_toolset, 14, NULL);
  COMP(itm_smg_9mm, 1, itm_uzi, 1, itm_tec9, 1, itm_calico, 1, itm_hk_mp5, 1,
     NULL);
  COMP(itm_processor, 2, NULL);
  COMP(itm_RAM, 2, NULL);
  COMP(itm_power_supply, 1, NULL);
//  COMP(itm_battery, 500, itm_plut_cell, 1, NULL);
//  COMP(itm_scrap, 30, NULL);
}
void game::recraft()
{
 if(u.lastrecipe == NULL)
 {
  popup("Craft something first");
 }
 else
 {
  try_and_make(u.lastrecipe);
 }
}
void game::try_and_make(recipe *making)
{
 if(can_make(making))
 {
  if (itypes[(making->result)]->m1 == LIQUID)
  {
   if (u.has_watertight_container() || u.has_matching_liquid(itypes[making->result]->id)) {
     make_craft(making);
   } else {
     popup("You don't have anything to store that liquid in!");
   }
  }
  else {
   make_craft(making);
  }
 }
 else
 {
  popup("You can't do that!");
 }
}
bool game::can_make(recipe *r)
{
 inventory crafting_inv = crafting_inventory();
 if((r->sk_primary != NULL && u.skillLevel(r->sk_primary) < r->difficulty) || (r->sk_secondary != NULL && u.skillLevel(r->sk_secondary) <= 0))
 {
 }
 // under the assumption that all comp and tool's array contains all the required stuffs at the start of the array

 // check all tools
 for(int i = 0 ; i < 20 ; i++)
 {
  // if current tool is null(size 0), assume that there is no more after it.
  if(r->tools[i].size()==0)
  {
   break;
  }
  bool has_tool = false;
  for(int j = 0 ; j < r->tools[i].size() ; j++)
  {
   itype_id type = r->tools[i][j].type;
   int req = r->tools[i][j].count;
   if((req<= 0 && crafting_inv.has_amount(type,1)) || (req > 0 && crafting_inv.has_charges(type,req)))
   {
    has_tool = true;
    break;
   }
  }
  if(!has_tool)
  {
   return false;
  }
 }
 // check all components
 for(int i = 0 ; i < 20 ; i++)
 {
  if(r->components[i].size() == 0)
  {
   break;
  }
  bool has_comp = false;
  for(int j = 0 ; j < r->components[i].size() ; j++)
  {
   itype_id type = r->components[i][j].type;
   int req = r->components[i][j].count;
   if (itypes[type]->count_by_charges() && req > 0)
   {
       if (crafting_inv.has_charges(type, req))
       {
           has_comp = true;
           break;
       }
   }
   else if (crafting_inv.has_amount(type, abs(req)))
   {
       has_comp = true;
       break;
   }
  }
  if(!has_comp)
  {
   return false;
  }
 }
 return true;
}

void game::craft()
{
 if (u.morale_level() < MIN_MORALE_CRAFT) {	// See morale.h
  add_msg("Your morale is too low to craft...");
  return;
 }

 int iMaxX = (VIEWX < 12) ? 80 : (VIEWX*2)+56;
 int iMaxY = (VIEWY < 12) ? 25 : (VIEWY*2)+1;

 WINDOW *w_head = newwin( 3, 80, (iMaxY > 25) ? (iMaxY-25)/2 : 0, (iMaxX > 80) ? (iMaxX-80)/2 : 0);
 WINDOW *w_data = newwin(22, 80, 3 + ((iMaxY > 25) ? (iMaxY-25)/2 : 0), (iMaxX > 80) ? (iMaxX-80)/2 : 0);
 craft_cat tab = CC_WEAPON;
 std::vector<recipe*> current;
 std::vector<bool> available;
 item tmp;
 int line = 0, xpos, ypos;
 bool redraw = true;
 bool done = false;
 InputEvent input;

 inventory crafting_inv = crafting_inventory();

 do {
  if (redraw) { // When we switch tabs, redraw the header
   redraw = false;
   line = 0;
   draw_recipe_tabs(w_head, tab);
   current.clear();
   available.clear();
// Set current to all recipes in the current tab; available are possible to make
   pick_recipes(current, available, tab);
  }

// Clear the screen of recipe data, and draw it anew
  werase(w_data);
  mvwprintz(w_data, 20, 5, c_white, "Press ? to describe object.  Press <ENTER> to attempt to craft object.");
  for (int i = 0; i < 80; i++) {
   mvwputch(w_data, 21, i, c_ltgray, LINE_OXOX);

   if (i < 21) {
    mvwputch(w_data, i, 0, c_ltgray, LINE_XOXO);
    mvwputch(w_data, i, 79, c_ltgray, LINE_XOXO);
   }
  }

  mvwputch(w_data, 21,  0, c_ltgray, LINE_XXOO); // _|
  mvwputch(w_data, 21, 79, c_ltgray, LINE_XOOX); // |_
  wrefresh(w_data);

  int recmin = 0, recmax = current.size();
  if(recmax > MAX_DISPLAYED_RECIPES){
   if (line <= recmin + 9) {
    for (int i = recmin; i < recmin + MAX_DISPLAYED_RECIPES; i++) {
     mvwprintz(w_data, i - recmin, 2, c_dkgray, "\
                               ");	// Clear the line
     if (i == line)
      mvwprintz(w_data, i - recmin, 2, (available[i] ? h_white : h_dkgray),
                itypes[current[i]->result]->name.c_str());
     else
      mvwprintz(w_data, i - recmin, 2, (available[i] ? c_white : c_dkgray),
                itypes[current[i]->result]->name.c_str());
    }
   } else if (line >= recmax - 9) {
    for (int i = recmax - MAX_DISPLAYED_RECIPES; i < recmax; i++) {
     mvwprintz(w_data, 18 + i - recmax, 2, c_ltgray, "\
                                ");	// Clear the line

     if (i == line)
       mvwprintz(w_data, 18 + i - recmax, 2, (available[i] ? h_white : h_dkgray),
                 itypes[current[i]->result]->name.c_str());
     else
      mvwprintz(w_data, 18 + i - recmax, 2, (available[i] ? c_white : c_dkgray),
                itypes[current[i]->result]->name.c_str());
    }
   } else {
    for (int i = line - 9; i < line + 9; i++) {
     mvwprintz(w_data, 9 + i - line, 2, c_ltgray, "\
                                ");	// Clear the line
     if (i == line)
       mvwprintz(w_data, 9 + i - line, 2, (available[i] ? h_white : h_dkgray),
                 itypes[current[i]->result]->name.c_str());
     else
      mvwprintz(w_data, 9 + i - line, 2, (available[i] ? c_white : c_dkgray),
                itypes[current[i]->result]->name.c_str());
    }
   }
  } else{
   for (int i = 0; i < current.size() && i < 23; i++) {
    if (i == line)
     mvwprintz(w_data, i, 2, (available[i] ? h_white : h_dkgray),
               itypes[current[i]->result]->name.c_str());
    else
     mvwprintz(w_data, i, 2, (available[i] ? c_white : c_dkgray),
               itypes[current[i]->result]->name.c_str());
   }
  }
  if (current.size() > 0) {
   nc_color col = (available[line] ? c_white : c_dkgray);
   mvwprintz(w_data, 0, 30, col, "Primary skill: %s",
             (current[line]->sk_primary == NULL ? "N/A" :
              current[line]->sk_primary->name().c_str()));
   mvwprintz(w_data, 1, 30, col, "Secondary skill: %s",
             (current[line]->sk_secondary == NULL ? "N/A" :
              current[line]->sk_secondary->name().c_str()));
   mvwprintz(w_data, 2, 30, col, "Difficulty: %d", current[line]->difficulty);
   if (current[line]->sk_primary == NULL)
    mvwprintz(w_data, 3, 30, col, "Your skill level: N/A");
   else
    mvwprintz(w_data, 3, 30, col, "Your skill level: %d",
              // Macs don't seem to like passing this as a class, so force it to int
              (int)u.skillLevel(current[line]->sk_primary));
   if (current[line]->time >= 1000)
    mvwprintz(w_data, 4, 30, col, "Time to complete: %d minutes",
              int(current[line]->time / 1000));
   else
    mvwprintz(w_data, 4, 30, col, "Time to complete: %d turns",
              int(current[line]->time / 100));
   mvwprintz(w_data, 5, 30, col, "Tools required:");
   if (current[line]->tools[0].size() == 0) {
    mvwputch(w_data, 6, 30, col, '>');
    mvwprintz(w_data, 6, 32, c_green, "NONE");
    ypos = 6;
   } else {
    ypos = 5;
// Loop to print the required tools
    for (int i = 0; i < 5 && current[line]->tools[i].size() > 0; i++) {
     ypos++;
     xpos = 32;
     mvwputch(w_data, ypos, 30, col, '>');

     for (int j = 0; j < current[line]->tools[i].size(); j++) {
      itype_id type = current[line]->tools[i][j].type;
      int charges = current[line]->tools[i][j].count;
      nc_color toolcol = c_red;

      if (charges < 0 && crafting_inv.has_amount(type, 1))
       toolcol = c_green;
      else if (charges > 0 && crafting_inv.has_charges(type, charges))
       toolcol = c_green;

      std::stringstream toolinfo;
      toolinfo << itypes[type]->name + " ";
      if (charges > 0)
       toolinfo << "(" << charges << " charges) ";
      std::string toolname = toolinfo.str();
      if (xpos + toolname.length() >= 80) {
       xpos = 32;
       ypos++;
      }
      mvwprintz(w_data, ypos, xpos, toolcol, toolname.c_str());
      xpos += toolname.length();
      if (j < current[line]->tools[i].size() - 1) {
       if (xpos >= 77) {
        xpos = 32;
        ypos++;
       }
       mvwprintz(w_data, ypos, xpos, c_white, "OR ");
       xpos += 3;
      }
     }
    }
   }
 // Loop to print the required components
   ypos++;
   mvwprintz(w_data, ypos, 30, col, "Components required:");
   for (int i = 0; i < 5; i++) {
    if (current[line]->components[i].size() > 0) {
     ypos++;
     mvwputch(w_data, ypos, 30, col, '>');
    }
    xpos = 32;
    for (int j = 0; j < current[line]->components[i].size(); j++) {
     int count = current[line]->components[i][j].count;
     itype_id type = current[line]->components[i][j].type;
     nc_color compcol = c_red;
     if (itypes[type]->count_by_charges() && count > 0)  {
      if (crafting_inv.has_charges(type, count))
       compcol = c_green;
     } else if (crafting_inv.has_amount(type, abs(count)))
      compcol = c_green;
     std::stringstream dump;
     dump << abs(count) << "x " << itypes[type]->name << " ";
     std::string compname = dump.str();
     if (xpos + compname.length() >= 80) {
      ypos++;
      xpos = 32;
     }
     mvwprintz(w_data, ypos, xpos, compcol, compname.c_str());
     xpos += compname.length();
     if (j < current[line]->components[i].size() - 1) {
      if (xpos >= 77) {
       ypos++;
       xpos = 32;
      }
      mvwprintz(w_data, ypos, xpos, c_white, "OR ");
      xpos += 3;
     }
    }
   }
  }

  wrefresh(w_data);
  input = get_input();
  switch (input) {
   case DirectionW:
   case DirectionUp:
    if (tab == CC_WEAPON)
     tab = CC_MISC;
    else
     tab = craft_cat(int(tab) - 1);
    redraw = true;
    break;
   case DirectionE:
   case DirectionDown:
    if (tab == CC_MISC)
     tab = CC_WEAPON;
    else
     tab = craft_cat(int(tab) + 1);
    redraw = true;
    break;
   case DirectionS:
    line++;
    break;
   case DirectionN:
    line--;
    break;
   case Confirm:
    if (!available[line])
     popup("You can't do that!");
    else
    // is player making a liquid? Then need to check for valid container
    if (itypes[current[line]->result]->m1 == LIQUID)
    {
     if (u.has_watertight_container() || u.has_matching_liquid(itypes[current[line]->result]->id)) {
             make_craft(current[line]);
             done = true;
             break;
     } else {
       popup("You don't have anything to store that liquid in!");
     }
    }
    else {
     make_craft(current[line]);
     done = true;
    }
    break;
   case Help:
    tmp = item(itypes[current[line]->result], 0);
    full_screen_popup(tmp.info(true).c_str());
    redraw = true;
    break;
  }
  if (line < 0)
   line = current.size() - 1;
  else if (line >= current.size())
   line = 0;
 } while (input != Cancel && !done);

 werase(w_head);
 werase(w_data);
 delwin(w_head);
 delwin(w_data);
 refresh_all();
}

void draw_recipe_tabs(WINDOW *w, craft_cat tab)
{
 werase(w);
 for (int i = 0; i < 80; i++)
  mvwputch(w, 2, i, c_ltgray, LINE_OXOX);

 mvwputch(w, 2,  0, c_ltgray, LINE_OXXO); // |^
 mvwputch(w, 2, 79, c_ltgray, LINE_OOXX); // ^|

 draw_tab(w,  2, "WEAPONS", (tab == CC_WEAPON) ? true : false);
 draw_tab(w, 13, "AMMO",    (tab == CC_AMMO)   ? true : false);
 draw_tab(w, 21, "FOOD",    (tab == CC_FOOD)   ? true : false);
 draw_tab(w, 29, "DRINKS",  (tab == CC_DRINK)  ? true : false);
 draw_tab(w, 39, "CHEMS",   (tab == CC_CHEM)   ? true : false);
 draw_tab(w, 48, "ELECTRONICS", (tab == CC_ELECTRONIC) ? true : false);
 draw_tab(w, 63, "ARMOR",   (tab == CC_ARMOR)  ? true : false);
 draw_tab(w, 72, "MISC",    (tab == CC_MISC)   ? true : false);

 wrefresh(w);
}

inventory game::crafting_inventory(){
 inventory crafting_inv;
 crafting_inv.form_from_map(this, point(u.posx, u.posy), PICKUP_RANGE);
 crafting_inv += u.inv;
 crafting_inv += u.weapon;
 if (u.has_bionic(bio_tools)) {
  item tools(itypes[itm_toolset], turn);
  tools.charges = u.power_level;
  crafting_inv += tools;
 }
 return crafting_inv;
}

void game::pick_recipes(std::vector<recipe*> &current,
                        std::vector<bool> &available, craft_cat tab)
{
 inventory crafting_inv = crafting_inventory();

 bool have_tool[5], have_comp[5];

 current.clear();
 available.clear();
 for (int i = 0; i < recipes.size(); i++) {
// Check if the category matches the tab, and we have the requisite skills
  if (recipes[i]->category == tab &&
      (recipes[i]->sk_primary == NULL ||
       u.skillLevel(recipes[i]->sk_primary) >= recipes[i]->difficulty) &&
      (recipes[i]->sk_secondary == NULL ||
       u.skillLevel(recipes[i]->sk_secondary) > 0))
  {
    if (recipes[i]->difficulty >= 0)
      current.push_back(recipes[i]);
  }
  available.push_back(false);
 }
 for (int i = 0; i < current.size() && i < 51; i++) {
//Check if we have the requisite tools and components
  for (int j = 0; j < 5; j++) {
   have_tool[j] = false;
   have_comp[j] = false;
   if (current[i]->tools[j].size() == 0)
    have_tool[j] = true;
   else {
    for (int k = 0; k < current[i]->tools[j].size(); k++) {
     itype_id type = current[i]->tools[j][k].type;
     int req = current[i]->tools[j][k].count;	// -1 => 1
     if ((req <= 0 && crafting_inv.has_amount (type,   1)) ||
         (req >  0 && crafting_inv.has_charges(type, req))   ) {
      have_tool[j] = true;
      k = current[i]->tools[j].size();
     }
    }
   }
   if (current[i]->components[j].size() == 0)
    have_comp[j] = true;
   else {
    for (int k = 0; k < current[i]->components[j].size() && !have_comp[j]; k++){
     itype_id type = current[i]->components[j][k].type;
     int count = current[i]->components[j][k].count;
     if (itypes[type]->count_by_charges() && count > 0) {
      if (crafting_inv.has_charges(type, count)) {
       have_comp[j] = true;
       k = current[i]->components[j].size();
      }
     } else if (crafting_inv.has_amount(type, abs(count))) {
      have_comp[j] = true;
      k = current[i]->components[j].size();
     }
    }
   }
  }
  if (have_tool[0] && have_tool[1] && have_tool[2] && have_tool[3] &&
      have_tool[4] && have_comp[0] && have_comp[1] && have_comp[2] &&
      have_comp[3] && have_comp[4])
   available[i] = true;
 }
}

void game::make_craft(recipe *making)
{
 u.assign_activity(ACT_CRAFT, making->time, making->id);
 u.moves = 0;
 u.lastrecipe = making;
}

void game::complete_craft()
{
 recipe* making = recipes[u.activity.index]; // Which recipe is it?

// # of dice is 75% primary skill, 25% secondary (unless secondary is null)
 int skill_dice = u.skillLevel(making->sk_primary) * 3;
 if (making->sk_secondary == NULL)
   skill_dice += u.skillLevel(making->sk_primary);
 else
   skill_dice += u.skillLevel(making->sk_secondary);
// Sides on dice is 16 plus your current intelligence
 int skill_sides = 16 + u.int_cur;

 int diff_dice = making->difficulty * 4; // Since skill level is * 4 also
 int diff_sides = 24;	// 16 + 8 (default intelligence)

 int skill_roll = dice(skill_dice, skill_sides);
 int diff_roll  = dice(diff_dice,  diff_sides);

 if (making->sk_primary)
  u.practice(making->sk_primary, making->difficulty * 5 + 20);
 if (making->sk_secondary)
  u.practice(making->sk_secondary, 5);

// Messed up badly; waste some components.
 if (making->difficulty != 0 && diff_roll > skill_roll * (1 + 0.1 * rng(1, 5))) {
  add_msg("You fail to make the %s, and waste some materials.",
          itypes[making->result]->name.c_str());
  for (int i = 0; i < 5; i++) {
   if (making->components[i].size() > 0) {
    std::vector<component> copy = making->components[i];
    for (int j = 0; j < copy.size(); j++)
     copy[j].count = rng(0, copy[j].count);
    consume_items(copy);
   }
   if (making->tools[i].size() > 0)
    consume_tools(making->tools[i]);
  }
  u.activity.type = ACT_NULL;
  return;
  // Messed up slightly; no components wasted.
 } else if (diff_roll > skill_roll) {
  add_msg("You fail to make the %s, but don't waste any materials.",
          itypes[making->result]->name.c_str());
  u.activity.type = ACT_NULL;
  return;
 }
// If we're here, the craft was a success!
// Use up the components and tools
 for (int i = 0; i < 5; i++) {
  if (making->components[i].size() > 0)
   consume_items(making->components[i]);
  if (making->tools[i].size() > 0)
   consume_tools(making->tools[i]);
 }

  // Set up the new item, and pick an inventory letter
 int iter = 0;
 item newit(itypes[making->result], turn, nextinv);

 // for food stacking
 if (newit.is_food())
  {
    int bday_tmp = turn % 3600;
    newit.bday = int(turn) + 3600 - bday_tmp;
  }
 if (!newit.craft_has_charges())
  newit.charges = 0;
 do {
  newit.invlet = nextinv;
  advance_nextinv();
  iter++;
 } while (u.has_item(newit.invlet) && iter < 52);
 //newit = newit.in_its_container(&itypes);
 if (newit.made_of(LIQUID))
  handle_liquid(newit, false, false);
 else {
// We might not have space for the item
  if (iter == 52 || u.volume_carried()+newit.volume() > u.volume_capacity()) {
   add_msg("There's no room in your inventory for the %s, so you drop it.",
             newit.tname().c_str());
   m.add_item(u.posx, u.posy, newit);
  } else if (u.weight_carried() + newit.volume() > u.weight_capacity()) {
   add_msg("The %s is too heavy to carry, so you drop it.",
           newit.tname().c_str());
   m.add_item(u.posx, u.posy, newit);
  } else {
   u.i_add(newit);
   add_msg("%c - %s", newit.invlet, newit.tname().c_str());
  }
 }
}

void game::consume_items(std::vector<component> components)
{
// For each set of components in the recipe, fill you_have with the list of all
// matching ingredients the player has.
 std::vector<component> player_has;
 std::vector<component> map_has;
 std::vector<component> mixed;
 std::vector<component> player_use;
 std::vector<component> map_use;
 std::vector<component> mixed_use;
 inventory map_inv;
 map_inv.form_from_map(this, point(u.posx, u.posy), PICKUP_RANGE);

 for (int i = 0; i < components.size(); i++) {
  itype_id type = components[i].type;
  int count = abs(components[i].count);
  bool pl = false, mp = false;


  if (itypes[type]->count_by_charges() && count > 0)
  {
   if (u.has_charges(type, count)) {
    player_has.push_back(components[i]);
    pl = true;
   }
   if (map_inv.has_charges(type, count)) {
    map_has.push_back(components[i]);
    mp = true;
   }
   if (!pl && !mp && u.charges_of(type) + map_inv.charges_of(type) >= count)
    mixed.push_back(components[i]);
  }

  else { // Counting by units, not charges

   if (u.has_amount(type, count)) {
    player_has.push_back(components[i]);
    pl = true;
   }
   if (map_inv.has_amount(type, count)) {
    map_has.push_back(components[i]);
    mp = true;
   }
   if (!pl && !mp && u.amount_of(type) + map_inv.amount_of(type) >= count)
    mixed.push_back(components[i]);

  }
 }

 if (player_has.size() + map_has.size() + mixed.size() == 1) { // Only 1 choice
  if (player_has.size() == 1)
   player_use.push_back(player_has[0]);
  else if (map_has.size() == 1)
   map_use.push_back(map_has[0]);
  else
   mixed_use.push_back(mixed[0]);

 } else { // Let the player pick which component they want to use
  std::vector<std::string> options; // List for the menu_vec below
// Populate options with the names of the items
  for (int i = 0; i < map_has.size(); i++) {
   std::string tmpStr = itypes[map_has[i].type]->name + " (nearby)";
   options.push_back(tmpStr);
  }
  for (int i = 0; i < player_has.size(); i++)
   options.push_back(itypes[player_has[i].type]->name);
  for (int i = 0; i < mixed.size(); i++) {
   std::string tmpStr = itypes[mixed[i].type]->name +" (on person & nearby)";
   options.push_back(tmpStr);
  }

  if (options.size() == 0) // This SHOULD only happen if cooking with a fire,
   return;                 // and the fire goes out.

// Get the selection via a menu popup
  int selection = menu_vec("Use which component?", options) - 1;
  if (selection < map_has.size())
   map_use.push_back(map_has[selection]);
  else if (selection < map_has.size() + player_has.size()) {
   selection -= map_has.size();
   player_use.push_back(player_has[selection]);
  } else {
   selection -= map_has.size() + player_has.size();
   mixed_use.push_back(mixed[selection]);
  }
 }

 for (int i = 0; i < player_use.size(); i++) {
  if (itypes[player_use[i].type]->count_by_charges() &&
      player_use[i].count > 0)
   u.use_charges(player_use[i].type, player_use[i].count);
  else
   u.use_amount(player_use[i].type, abs(player_use[i].count),
                   (player_use[i].count < 0));
 }
 for (int i = 0; i < map_use.size(); i++) {
  if (itypes[map_use[i].type]->count_by_charges() &&
      map_use[i].count > 0)
   m.use_charges(point(u.posx, u.posy), PICKUP_RANGE,
                    map_use[i].type, map_use[i].count);
  else
   m.use_amount(point(u.posx, u.posy), PICKUP_RANGE,
                   map_use[i].type, abs(map_use[i].count),
                   (map_use[i].count < 0));
 }
 for (int i = 0; i < mixed_use.size(); i++) {
  if (itypes[mixed_use[i].type]->count_by_charges() &&
      mixed_use[i].count > 0) {
   int from_map = mixed_use[i].count - u.charges_of(mixed_use[i].type);
   u.use_charges(mixed_use[i].type, u.charges_of(mixed_use[i].type));
   m.use_charges(point(u.posx, u.posy), PICKUP_RANGE,
                    mixed_use[i].type, from_map);
  } else {
   bool in_container = (mixed_use[i].count < 0);
   int from_map = abs(mixed_use[i].count) - u.amount_of(mixed_use[i].type);
   u.use_amount(mixed_use[i].type, u.amount_of(mixed_use[i].type),
                   in_container);
   m.use_amount(point(u.posx, u.posy), PICKUP_RANGE,
                   mixed_use[i].type, from_map, in_container);
  }
 }
}

void game::consume_tools(std::vector<component> tools)
{
 bool found_nocharge = false;
 inventory map_inv;
 map_inv.form_from_map(this, point(u.posx, u.posy), PICKUP_RANGE);
 std::vector<component> player_has;
 std::vector<component> map_has;
// Use charges of any tools that require charges used
 for (int i = 0; i < tools.size() && !found_nocharge; i++) {
  itype_id type = tools[i].type;
  if (tools[i].count > 0) {
   int count = tools[i].count;
   if (u.has_charges(type, count))
    player_has.push_back(tools[i]);
   if (map_inv.has_charges(type, count))
    map_has.push_back(tools[i]);
  } else if (u.has_amount(type, 1) || map_inv.has_amount(type, 1))
   found_nocharge = true;
 }
 if (found_nocharge)
  return; // Default to using a tool that doesn't require charges

 if (player_has.size() == 1 && map_has.size() == 0)
  u.use_charges(player_has[0].type, player_has[0].count);
 else if (map_has.size() == 1 && player_has.size() == 0)
  m.use_charges(point(u.posx, u.posy), PICKUP_RANGE,
                   map_has[0].type, map_has[0].count);
 else { // Variety of options, list them and pick one
// Populate the list
  std::vector<std::string> options;
  for (int i = 0; i < map_has.size(); i++) {
   std::string tmpStr = itypes[map_has[i].type]->name + " (nearby)";
   options.push_back(tmpStr);
  }
  for (int i = 0; i < player_has.size(); i++)
   options.push_back(itypes[player_has[i].type]->name);

  if (options.size() == 0) // This SHOULD only happen if cooking with a fire,
   return;                 // and the fire goes out.

// Get selection via a popup menu
  int selection = menu_vec("Use which tool?", options) - 1;
  if (selection < map_has.size())
   m.use_charges(point(u.posx, u.posy), PICKUP_RANGE,
                    map_has[selection].type, map_has[selection].count);
  else {
   selection -= map_has.size();
   u.use_charges(player_has[selection].type, player_has[selection].count);
  }
 }
}

void game::disassemble()
{
  char ch = inv("Disassemble item:");
  if (ch == 27) {
    add_msg("Never mind.");
    return;
  }
  if (!u.has_item(ch)) {
    add_msg("You don't have item '%c'!", ch);
    return;
  }

  item* dis_item = &u.i_at(ch);

  if (OPTIONS[OPT_QUERY_DISASSEMBLE] && !(query_yn("Really disassemble your %s?", dis_item->tname(this).c_str())))
    return;

  for (int i = 0; i < recipes.size(); i++) {
    if (dis_item->type == itypes[recipes[i]->result] && recipes[i]->reversible)
    // ok, a valid recipe exists for the item, and it is reversible
    // assign the activity
    {
      // check tools are available
      // loop over the tools and see what's required...again
      inventory crafting_inv = crafting_inventory();
      bool have_tool[5];
      for (int j = 0; j < 5; j++)
      {
        have_tool[j] = false;
        if (recipes[i]->tools[j].size() == 0) // no tools required, may change this
          have_tool[j] = true;
        else
        {
          for (int k = 0; k < recipes[i]->tools[j].size(); k++)
          {
            itype_id type = recipes[i]->tools[j][k].type;
            int req = recipes[i]->tools[j][k].count;	// -1 => 1

            if ((req <= 0 && crafting_inv.has_amount (type, 1)) ||
              (req >  0 && crafting_inv.has_charges(type, req)))
            {
              have_tool[j] = true;
              k = recipes[i]->tools[j].size();
            }
            // if crafting recipe required a welder, disassembly requires a hacksaw or super toolkit
            if (type == itm_welder)
            {
              if (crafting_inv.has_amount(itm_hacksaw, 1) ||
                  crafting_inv.has_amount(itm_toolset, 1))
                have_tool[j] = true;
              else
                have_tool[j] = false;
            }
          }

          if (!have_tool[j])
          {
            int req = recipes[i]->tools[j][0].count;
            if (recipes[i]->tools[j][0].type == itm_welder)
                add_msg("You need a hack saw to disassemble this.");
            else
            {
              if (req <= 0)
              {
                add_msg("You need a %s to disassemble this.",
                itypes[recipes[i]->tools[j][0].type]->name.c_str());
              }
              else
              {
                add_msg("You need a %s with %d charges to disassemble this.",
                itypes[recipes[i]->tools[j][0].type]->name.c_str(), req);
              }
            }
          }
        }
      }
      // all tools present, so assign the activity
      if (have_tool[0] && have_tool[1] && have_tool[2] && have_tool[3] &&
      have_tool[4])
      {
        u.assign_activity(ACT_DISASSEMBLE, recipes[i]->time, recipes[i]->id);
        u.moves = 0;
        std::vector<int> dis_items;
        dis_items.push_back(ch);
        u.activity.values = dis_items;
      }
      return; // recipe exists, but no tools, so do not start disassembly
    }
  }
  // no recipe exists, or the item cannot be disassembled
  add_msg("This item cannot be disassembled!");
}

void game::complete_disassemble()
{
  // which recipe was it?
  recipe* dis = recipes[u.activity.index]; // Which recipe is it?
  item* dis_item = &u.i_at(u.activity.values[0]);

  add_msg("You disassemble the item into its components.");
  // remove any batteries or ammo first
    if (dis_item->is_gun() && dis_item->curammo != NULL && dis_item->ammo_type() != AT_NULL)
    {
      item ammodrop;
      ammodrop = item(dis_item->curammo, turn);
      ammodrop.charges = dis_item->charges;
      if (ammodrop.made_of(LIQUID))
        handle_liquid(ammodrop, false, false);
      else
        m.add_item(u.posx, u.posy, ammodrop);
    }
    if (dis_item->is_tool() && dis_item->charges > 0 && dis_item->ammo_type() != AT_NULL)
    {
      item ammodrop;
      ammodrop = item(itypes[default_ammo(dis_item->ammo_type())], turn);
      ammodrop.charges = dis_item->charges;
      if (ammodrop.made_of(LIQUID))
        handle_liquid(ammodrop, false, false);
      else
        m.add_item(u.posx, u.posy, ammodrop);
    }
    u.i_rem(u.activity.values[0]);  // remove the item

  // consume tool charges
  for (int j = 0; j < 5; j++)
  {
    if (dis->tools[j].size() > 0)
    consume_tools(dis->tools[j]);
  }

  // add the components to the map
  // Player skills should determine how many components are returned

  // adapting original crafting formula to check if disassembly was successful
  // # of dice is 75% primary skill, 25% secondary (unless secondary is null)
  int skill_dice = 2 + u.skillLevel(dis->sk_primary) * 3;
   if (dis->sk_secondary == NULL)
     skill_dice += u.skillLevel(dis->sk_primary);
   else
     skill_dice += u.skillLevel(dis->sk_secondary);
  // Sides on dice is 16 plus your current intelligence
   int skill_sides = 16 + u.int_cur;

   int diff_dice = dis->difficulty;
   int diff_sides = 24;	// 16 + 8 (default intelligence)

   // disassembly only nets a bit of practice
   if (dis->sk_primary)
    u.practice(dis->sk_primary, (dis->difficulty) * 2);
   if (dis->sk_secondary)
    u.practice(dis->sk_secondary, 2);

  for (int j = 0; j < 5; j++)
  {
    if (dis->components[j].size() != 0)
    {
      int compcount = dis->components[j][0].count;
      bool comp_success = (dice(skill_dice, skill_sides) > dice(diff_dice,  diff_sides));
      do
      {
        item newit(itypes[dis->components[j][0].type], turn);
        // skip item addition if component is a consumable like superglue
        if (dis->components[j][0].type == itm_superglue || dis->components[j][0].type == itm_duct_tape)
          compcount--;
        else
        {
          if (newit.count_by_charges())
          {
            if (dis->difficulty == 0 || comp_success)
              m.add_item(u.posx, u.posy, itypes[dis->components[j][0].type], 0, compcount);
            else
              add_msg("You fail to recover a component.");
            compcount = 0;
          } else
          {
            if (dis->difficulty == 0 || comp_success)
              m.add_item(u.posx, u.posy, newit);
            else
              add_msg("You fail to recover a component.");
            compcount--;
          }
        }
      } while (compcount > 0);
    }
  }
}
