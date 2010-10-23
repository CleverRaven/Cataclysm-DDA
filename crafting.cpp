#include <string>
#include <sstream>
#include "keypress.h"
#include "game.h"
#include "output.h"
#include "crafting.h"
#include "setvector.h"

void draw_recipe_tabs(WINDOW *w, craft_cat tab);

void game::init_recipes()
{
 int id = -1;
 int tl, cl;

 #define RECIPE(result, cat, sk1, sk2, diff, time) \
  tl = 0; cl = 0; id++;\
  recipes.push_back( recipe(id, result, cat, sk1, sk2, diff, time) )
 #define TOOL(...)	setvector(recipes[id].tools[tl],      __VA_ARGS__); tl++
 #define COMP(...)	setvector(recipes[id].components[cl], __VA_ARGS__); cl++

/* A recipe will not appear in your menu until your level in the primary skill
 * is at least equal to the difficulty.  At that point, your chance of success
 * is still not great; a good 25% improvement over the difficulty is important
 */

// WEAPONS

 RECIPE(itm_spear_wood, CC_WEAPON, sk_null, sk_null, 0, 800);
  TOOL(itm_hatchet, -1, itm_knife_steak, -1, itm_knife_butcher, -1,
	itm_knife_combat, -1, itm_machete, -1, NULL);
  COMP(itm_stick, 1, itm_broom, 1, itm_mop, 1, itm_2x4, 1, NULL);

 RECIPE(itm_nailboard, CC_WEAPON, sk_null, sk_null, 0, 1000);
  TOOL(itm_hatchet, -1, itm_hammer, -1, itm_rock, -1, NULL);
  COMP(itm_2x4, 1, itm_bat, 1, NULL);
  COMP(itm_nail, 6, NULL);

 RECIPE(itm_molotov, CC_WEAPON, sk_null, sk_null, 0, 500);
  COMP(itm_rag, 1, NULL);
  COMP(itm_whiskey, 1, itm_vodka, 1, itm_rum, 1, itm_tequila, 1,
       itm_gasoline, 1, NULL);

 RECIPE(itm_shotgun_sawn, CC_WEAPON, sk_gun, sk_null, 1, 500);
  TOOL(itm_hacksaw, -1,  NULL);
  COMP(itm_shotgun_d, 1, itm_remington_870, 1, itm_mossberg_500, 1,
       itm_saiga_12, 1, NULL);

 RECIPE(itm_bolt_wood, CC_WEAPON, sk_mechanics, sk_gun, 1, 5000);
  TOOL(itm_hatchet, -1, itm_knife_steak, -1, itm_knife_butcher, -1,
	itm_knife_combat, -1, itm_machete, -1, NULL);
  COMP(itm_stick, 1, itm_broom, 1, itm_mop, 1, itm_2x4, 1, itm_bee_sting, 1,
       NULL);

 RECIPE(itm_crossbow, CC_WEAPON, sk_mechanics, sk_gun, 3, 15000);
  TOOL(itm_wrench, -1, NULL);
  TOOL(itm_screwdriver, -1, NULL);
  COMP(itm_2x4, 1, itm_stick, 4, NULL);
  COMP(itm_hose, 1, NULL);

 RECIPE(itm_rifle_22, CC_WEAPON, sk_mechanics, sk_gun, 3, 12000);
  TOOL(itm_hacksaw, -1, NULL);
  TOOL(itm_screwdriver, -1, NULL);
  COMP(itm_pipe, 1, NULL);
  COMP(itm_2x4, 1, NULL);

 RECIPE(itm_rifle_9mm, CC_WEAPON, sk_mechanics, sk_gun, 3, 14000);
  TOOL(itm_hacksaw, -1, NULL);
  TOOL(itm_screwdriver, -1, NULL);
  COMP(itm_pipe, 1, NULL);
  COMP(itm_2x4, 1, NULL);

 RECIPE(itm_smg_9mm, CC_WEAPON, sk_mechanics, sk_gun, 5, 18000);
  TOOL(itm_hacksaw, -1, NULL);
  TOOL(itm_screwdriver, -1, NULL);
  TOOL(itm_hammer, -1, itm_rock, -1, itm_hatchet, -1, NULL);
  COMP(itm_pipe, 1, NULL);
  COMP(itm_2x4, 2, NULL);
  COMP(itm_nail, 4, NULL);

 RECIPE(itm_smg_45, CC_WEAPON, sk_mechanics, sk_gun, 5, 20000);
  TOOL(itm_hacksaw, -1, NULL);
  TOOL(itm_screwdriver, -1, NULL);
  TOOL(itm_hammer, -1, itm_rock, -1, itm_hatchet, -1, NULL);
  COMP(itm_pipe, 1, NULL);
  COMP(itm_2x4, 2, NULL);
  COMP(itm_nail, 4, NULL);

 RECIPE(itm_grenade, CC_WEAPON, sk_mechanics, sk_null, 2, 5000);
  TOOL(itm_screwdriver, -1, NULL);
  COMP(itm_superglue, 1, itm_string_36, 1, NULL);
  COMP(itm_can_food, 1, itm_can_drink, 1, itm_canister_empty, 1, NULL);
  COMP(itm_nail, 30, itm_bb, 100, NULL);
  COMP(itm_shot_bird, 6, itm_shot_00, 3, itm_shot_slug, 2,
       itm_gasoline, 1, NULL);

 RECIPE(itm_chainsaw_off, CC_WEAPON, sk_mechanics, sk_null, 4, 20000);
  TOOL(itm_screwdriver, -1, NULL);
  TOOL(itm_hammer, -1, itm_hatchet, -1, NULL);
  TOOL(itm_wrench, -1, NULL);
  COMP(itm_motor, 1, NULL);
  COMP(itm_chain, 1, NULL);

 RECIPE(itm_smokebomb, CC_WEAPON, sk_cooking, sk_mechanics, 3, 7500);
  TOOL(itm_screwdriver, -1, itm_wrench, -1, NULL);
  COMP(itm_water, 1, itm_water_dirty, 1, itm_salt_water, 1, NULL);
  COMP(itm_candy, 1, itm_cola, 1, NULL);
  COMP(itm_vitamins, 10, itm_aspirin, 8, NULL);
  COMP(itm_canister_empty, 1, itm_can_food, 1, NULL);
  COMP(itm_superglue, 1, NULL);

 RECIPE(itm_gasbomb, CC_WEAPON, sk_cooking, sk_mechanics, 4, 8000);
  TOOL(itm_screwdriver, -1, itm_wrench, -1, NULL);
  COMP(itm_bleach, 2, NULL);
  COMP(itm_ammonia, 2, NULL);
  COMP(itm_canister_empty, 1, itm_can_food, 1, NULL);
  COMP(itm_superglue, 1, NULL);

 RECIPE(itm_landmine, CC_WEAPON, sk_traps, sk_mechanics, 5, 10000);
  TOOL(itm_screwdriver, -1, NULL);
  COMP(itm_superglue, 1, NULL);
  COMP(itm_can_food, 1, itm_steel_chunk, 1, itm_canister_empty, 1, NULL);
  COMP(itm_nail, 100, itm_bb, 100, NULL);
  COMP(itm_shot_bird, 30, itm_shot_00, 18, itm_shot_slug, 15, itm_gasoline, 3,
       itm_grenade, 1, NULL);

 RECIPE(itm_mininuke, CC_WEAPON, sk_mechanics, sk_electronics, 10, 40000);
  TOOL(itm_screwdriver, -1, NULL);
  TOOL(itm_wrench, -1, NULL);
  COMP(itm_can_food, 2, itm_steel_chunk, 2, itm_canister_empty, 1, NULL);
  COMP(itm_plut_cell, 6, NULL);
  COMP(itm_battery, 2, NULL);
  COMP(itm_power_supply, 1, NULL);

// FOOD

 RECIPE(itm_meat_cooked, CC_FOOD, sk_cooking, sk_null, 0, 5000);
  TOOL(itm_hotplate, 7, NULL);
  TOOL(itm_pan, -1, itm_pot, -1, NULL);
  COMP(itm_meat, 1, NULL);

 RECIPE(itm_veggy_cooked, CC_FOOD, sk_cooking, sk_null, 0, 4000);
  TOOL(itm_hotplate, 5, NULL);
  TOOL(itm_pan, -1, itm_pot, -1, NULL);
  COMP(itm_veggy, 1, NULL);

 RECIPE(itm_spaghetti_cooked, CC_FOOD, sk_cooking, sk_null, 0, 10000);
  TOOL(itm_hotplate, 4, NULL);
  TOOL(itm_pot, -1, NULL);
  COMP(itm_spaghetti_raw, 1, NULL);
  COMP(itm_water, 1, itm_water_dirty, 1, NULL);

 RECIPE(itm_macaroni_cooked, CC_FOOD, sk_cooking, sk_null, 1, 10000);
  TOOL(itm_hotplate, 4, NULL);
  TOOL(itm_pot, -1, NULL);
  COMP(itm_macaroni_raw, 1, NULL);
  COMP(itm_water, 1, itm_water_dirty, 1, NULL);

 RECIPE(itm_jerky, CC_FOOD, sk_cooking, sk_null, 2, 30000);
  TOOL(itm_hotplate, 10, NULL);
  COMP(itm_salt_water, 1, NULL);
  COMP(itm_meat, 1, NULL);

 RECIPE(itm_V8, CC_FOOD, sk_cooking, sk_null, 2, 5000);
  COMP(itm_can_drink, 1, NULL);
  COMP(itm_tomato, 1, NULL);
  COMP(itm_broccoli, 1, NULL);
  COMP(itm_zucchini, 1, NULL);

 RECIPE(itm_meth, CC_FOOD, sk_cooking, sk_null, 4, 20000);
  TOOL(itm_hotplate, 15, NULL);
  TOOL(itm_bottle_glass, -1, itm_hose, -1, NULL);
  COMP(itm_dayquil, 2, itm_royal_jelly, 1, NULL);
  COMP(itm_aspirin, 40, NULL);
  COMP(itm_caffeine, 20, itm_adderall, 5, itm_energy_drink, 2, NULL);

 RECIPE(itm_royal_jelly, CC_FOOD, sk_cooking, sk_null, 5, 5000);
  COMP(itm_honeycomb, 1, NULL);
  COMP(itm_bleach, 2, itm_purifier, 1, NULL);

 RECIPE(itm_heroin, CC_FOOD, sk_cooking, sk_null, 6, 2000);
  TOOL(itm_hotplate, 3, itm_lighter, 25, NULL);
  TOOL(itm_pan, -1, itm_pot, -1, NULL);
  COMP(itm_salt_water, 1, NULL);
  COMP(itm_oxycodone, 40, NULL);

 RECIPE(itm_mutagen, CC_FOOD, sk_cooking, sk_firstaid, 8, 10000);
  TOOL(itm_hotplate, 25, NULL);
  COMP(itm_meat_tainted, 3, itm_veggy_tainted, 5, itm_fetus, 1, itm_arm, 2,
       itm_leg, 2, NULL);
  COMP(itm_bleach, 2, NULL);
  COMP(itm_ammonia, 1, NULL);

 RECIPE(itm_purifier, CC_FOOD, sk_cooking, sk_firstaid, 9, 10000);
  TOOL(itm_hotplate, 25, NULL);
  COMP(itm_royal_jelly, 3, itm_mutagen, 1, NULL);
  COMP(itm_bleach, 3, NULL);
  COMP(itm_ammonia, 2, NULL);

// ELECTRONICS

 RECIPE(itm_antenna, CC_ELECTRONIC, sk_null, sk_null, 0, 3000);
  TOOL(itm_hacksaw, -1, NULL);
  COMP(itm_radio, 1, itm_two_way_radio, 1, itm_motor, 1, itm_knife_butter, 2,
       NULL);

 RECIPE(itm_amplifier, CC_ELECTRONIC, sk_electronics, sk_null, 2, 4000);
  TOOL(itm_screwdriver, -1, NULL);
  COMP(itm_flashlight, 1, itm_radio, 1, itm_two_way_radio, 1, itm_geiger, 1,
       itm_goggles_nv, 1, itm_transponder, 2, NULL);

 RECIPE(itm_power_supply, CC_ELECTRONIC, sk_electronics, sk_null, 2, 6500);
  TOOL(itm_screwdriver, -1, NULL);
  TOOL(itm_soldering_iron, 3, NULL);
  COMP(itm_amplifier, 2, itm_soldering_iron, 1, itm_electrohack, 1,
       itm_battery, 800, itm_geiger, 1, NULL);

 RECIPE(itm_receiver, CC_ELECTRONIC, sk_electronics, sk_null, 3, 12000);
  TOOL(itm_screwdriver, -1, NULL);
  TOOL(itm_soldering_iron, 4, NULL);
  COMP(itm_amplifier, 2, itm_radio, 1, itm_two_way_radio, 1, NULL);

 RECIPE(itm_transponder, CC_ELECTRONIC, sk_electronics, sk_null, 3, 14000);
  TOOL(itm_screwdriver, -1, NULL);
  TOOL(itm_soldering_iron, 7, NULL);
  COMP(itm_receiver, 3, itm_two_way_radio, 1, NULL);

 RECIPE(itm_flashlight, CC_ELECTRONIC, sk_electronics, sk_null, 1, 10000);
  COMP(itm_amplifier, 1, NULL);
  COMP(itm_bottle_plastic, 1, itm_bottle_glass, 1, itm_can_drink, 1, NULL);

 RECIPE(itm_soldering_iron, CC_ELECTRONIC, sk_electronics, sk_null, 1, 20000);
  COMP(itm_screwdriver, 1, itm_antenna, 1, itm_xacto, 1, itm_knife_butter, 1,
       NULL);
  COMP(itm_power_supply, 1, NULL);

 RECIPE(itm_battery, CC_ELECTRONIC, sk_electronics, sk_mechanics, 2, 5000);
  TOOL(itm_screwdriver, -1, NULL);
  COMP(itm_ammonia, 1, itm_lemon, 1, NULL);
  COMP(itm_steel_chunk, 1, itm_knife_butter, 1, itm_knife_steak, 1,
       itm_bolt_steel, 1, NULL);
  COMP(itm_can_drink, 1, itm_can_food, 1, NULL);

 RECIPE(itm_radio, CC_ELECTRONIC, sk_electronics, sk_null, 2, 25000);
  TOOL(itm_screwdriver, -1, NULL);
  TOOL(itm_soldering_iron, 10, NULL);
  COMP(itm_receiver, 1, NULL);
  COMP(itm_antenna, 1, NULL);

 RECIPE(itm_water_purifier, CC_ELECTRONIC, sk_mechanics,sk_electronics,3,25000);
  TOOL(itm_screwdriver, -1, NULL);
  COMP(itm_hotplate, 1, NULL);
  COMP(itm_bottle_glass, 2, itm_bottle_plastic, 5, NULL);
  COMP(itm_hose, 1, NULL);

 RECIPE(itm_hotplate, CC_ELECTRONIC, sk_electronics, sk_null, 3, 30000);
  TOOL(itm_screwdriver, -1, NULL);
  COMP(itm_soldering_iron, 1, itm_amplifier, 1, NULL);
  COMP(itm_pan, 1, itm_pot, 1, itm_knife_butcher, 2, itm_knife_steak, 6,
       itm_knife_butter, 6, itm_muffler, 1, NULL);

 RECIPE(itm_two_way_radio, CC_ELECTRONIC, sk_electronics, sk_null, 4, 30000);
  TOOL(itm_screwdriver, -1, NULL);
  TOOL(itm_soldering_iron, 14, NULL);
  COMP(itm_amplifier, 1, NULL);
  COMP(itm_transponder, 1, NULL);
  COMP(itm_receiver, 1, NULL);
  COMP(itm_antenna, 1, NULL);

 RECIPE(itm_electrohack, CC_ELECTRONIC, sk_electronics, sk_computer, 4, 35000);
  TOOL(itm_screwdriver, -1, NULL);
  TOOL(itm_soldering_iron, 10, NULL);
  COMP(itm_processor, 1, NULL);
  COMP(itm_RAM, 1, NULL);

 RECIPE(itm_geiger, CC_ELECTRONIC, sk_electronics, sk_null, 5, 35000);
  TOOL(itm_screwdriver, -1, NULL);
  TOOL(itm_soldering_iron, 14, NULL);
  COMP(itm_power_supply, 1, NULL);
  COMP(itm_amplifier, 2, NULL);

 RECIPE(itm_teleporter, CC_ELECTRONIC, sk_electronics, sk_null, 8, 50000);
  TOOL(itm_screwdriver, -1, NULL);
  TOOL(itm_wrench, -1, NULL);
  TOOL(itm_soldering_iron, 16, NULL);
  COMP(itm_power_supply, 3, itm_plut_cell, 5, NULL);
  COMP(itm_amplifier, 3, NULL);
  COMP(itm_transponder, 3, NULL);

// ARMOR

 RECIPE(itm_mocassins, CC_ARMOR, sk_tailor, sk_null, 1, 30000);
  TOOL(itm_sewing_kit,  5, NULL);
  COMP(itm_fur, 2, NULL);

 RECIPE(itm_boots, CC_ARMOR, sk_tailor, sk_null, 2, 35000);
  TOOL(itm_sewing_kit, 10, NULL);
  COMP(itm_leather, 2, NULL);

 RECIPE(itm_jeans, CC_ARMOR, sk_tailor, sk_null, 2, 45000);
  TOOL(itm_sewing_kit, 10, NULL);
  COMP(itm_rag, 6, NULL);

 RECIPE(itm_pants_cargo, CC_ARMOR, sk_tailor, sk_null, 3, 48000);
  TOOL(itm_sewing_kit, 16, NULL);
  COMP(itm_rag, 8, NULL);

 RECIPE(itm_pants_leather, CC_ARMOR, sk_tailor, sk_null, 4, 50000);
  TOOL(itm_sewing_kit, 10, NULL);
  COMP(itm_leather, 6, NULL);

 RECIPE(itm_tank_top, CC_ARMOR, sk_tailor, sk_null, 2, 38000);
  TOOL(itm_sewing_kit, 4, NULL);
  COMP(itm_rag, 4, NULL);

 RECIPE(itm_hoodie, CC_ARMOR, sk_tailor, sk_null, 3, 40000);
  TOOL(itm_sewing_kit, 14, NULL);
  COMP(itm_rag, 8, NULL);

 RECIPE(itm_trenchcoat, CC_ARMOR, sk_tailor, sk_null, 3, 42000);
  TOOL(itm_sewing_kit, 24, NULL);
  COMP(itm_rag, 10, NULL);

 RECIPE(itm_coat_fur, CC_ARMOR, sk_tailor, sk_null, 4, 100000);
  TOOL(itm_sewing_kit, 20, NULL);
  COMP(itm_fur, 10, NULL);

 RECIPE(itm_jacket_leather, CC_ARMOR, sk_tailor, sk_null, 5, 150000);
  TOOL(itm_sewing_kit, 30, NULL);
  COMP(itm_leather, 8, NULL);

 RECIPE(itm_gloves_light, CC_ARMOR, sk_tailor, sk_null, 1, 10000);
  TOOL(itm_sewing_kit, 2, NULL);
  COMP(itm_rag, 2, NULL);

 RECIPE(itm_gloves_fingerless, CC_ARMOR, sk_tailor, sk_null, 3, 16000);
  TOOL(itm_sewing_kit, 6, NULL);
  COMP(itm_leather, 2, NULL);

 RECIPE(itm_mask_filter, CC_ARMOR, sk_mechanics, sk_tailor, 1, 5000);
  COMP(itm_bottle_plastic, 1, itm_bag_plastic, 2, NULL);
  COMP(itm_muffler, 1, itm_bandana, 2, itm_rag, 2, itm_wrapper, 4, NULL);

 RECIPE(itm_mask_gas, CC_ARMOR, sk_tailor, sk_null, 3, 20000);
  TOOL(itm_wrench, -1, NULL);
  COMP(itm_goggles_swim, 2, itm_goggles_ski, 1, NULL);
  COMP(itm_mask_filter, 3, itm_muffler, 1, NULL);
  COMP(itm_hose, 1, NULL);

 RECIPE(itm_glasses_safety, CC_ARMOR, sk_tailor, sk_null, 1, 8000);
  COMP(itm_string_36, 1, itm_string_6, 2, NULL);
  COMP(itm_bottle_plastic, 1, NULL);

 RECIPE(itm_goggles_nv, CC_ARMOR, sk_electronics, sk_tailor, 5, 40000);
  TOOL(itm_screwdriver, -1, NULL);
  COMP(itm_goggles_ski, 1, itm_goggles_welding, 1, itm_mask_gas, 1, NULL);
  COMP(itm_power_supply, 1, NULL);
  COMP(itm_amplifier, 3, NULL);

 RECIPE(itm_hat_fur, CC_ARMOR, sk_tailor, sk_null, 2, 40000);
  TOOL(itm_sewing_kit, 8, NULL);
  COMP(itm_fur, 3, NULL);

 RECIPE(itm_helmet_chitin, CC_ARMOR, sk_tailor, sk_null, 6, 200000);
  COMP(itm_string_36, 1, itm_string_6, 5, NULL);
  COMP(itm_chitin_piece, 5, NULL);

 RECIPE(itm_armor_chitin, CC_ARMOR, sk_tailor, sk_null,  7, 500000);
  COMP(itm_string_36, 2, itm_string_6, 12, NULL);
  COMP(itm_chitin_piece, 15, NULL);

 RECIPE(itm_backpack, CC_ARMOR, sk_tailor, sk_null, 3, 50000);
  TOOL(itm_sewing_kit, 20, NULL);
  COMP(itm_rag, 20, itm_fur, 16, itm_leather, 12, NULL);

// MISC

 RECIPE(itm_superglue, CC_MISC, sk_cooking, sk_null, 2, 12000);
  TOOL(itm_hotplate, 5, NULL);
  COMP(itm_water, 1, itm_water_dirty, 1, NULL);
  COMP(itm_milk, 2, itm_bleach, 1, itm_ant_egg, 1, NULL);

 RECIPE(itm_2x4, CC_MISC, sk_null, sk_null, 0, 8000);
  TOOL(itm_saw, -1, NULL);
  COMP(itm_stick, 1, NULL);

 RECIPE(itm_crowbar, CC_MISC, sk_mechanics, sk_null, 1, 1000);
  TOOL(itm_hatchet, -1, itm_hammer, -1, itm_rock, -1, NULL);
  COMP(itm_pipe, 1, NULL);

 RECIPE(itm_tripwire, CC_MISC, sk_traps, sk_null, 1, 500);
  COMP(itm_string_36, 1, NULL);
  COMP(itm_superglue, 1, NULL);

 RECIPE(itm_board_trap, CC_MISC, sk_traps, sk_null, 2, 2500);
  TOOL(itm_hatchet, -1, itm_hammer, -1, itm_rock, -1, NULL);
  COMP(itm_2x4, 3, NULL);
  COMP(itm_nail, 20, NULL);

 RECIPE(itm_beartrap, CC_MISC, sk_mechanics, sk_traps, 2, 3000);
  TOOL(itm_wrench, -1, NULL);
  COMP(itm_steel_chunk, 2, NULL);

 RECIPE(itm_crossbow_trap, CC_MISC, sk_mechanics, sk_traps, 3, 4500);
  COMP(itm_crossbow, 1, NULL);
  COMP(itm_bolt_steel, 1, NULL);
  COMP(itm_string_36, 1, itm_string_6, 2, NULL);

 RECIPE(itm_shotgun_trap, CC_MISC, sk_mechanics, sk_traps, 3, 5000);
  COMP(itm_shotgun_sawn, 1, NULL);
  COMP(itm_shot_00, 2, NULL);
  COMP(itm_string_36, 1, itm_string_6, 2, NULL);

 RECIPE(itm_blade_trap, CC_MISC, sk_mechanics, sk_traps, 4, 8000);
  TOOL(itm_screwdriver, -1, NULL);
  COMP(itm_motor, 1, NULL);
  COMP(itm_machete, 1, NULL);
  COMP(itm_string_36, 1, NULL);

 RECIPE(itm_bandages, CC_MISC, sk_firstaid, sk_null, 1, 500);
  COMP(itm_rag, 1, NULL);
  COMP(itm_superglue, 1, NULL);

 RECIPE(itm_silencer, CC_MISC, sk_mechanics, sk_null, 1, 650);
  TOOL(itm_hacksaw, -1, NULL);
  COMP(itm_muffler, 1, itm_rag, 4, NULL);
  COMP(itm_pipe, 1, NULL);

 RECIPE(itm_pheromone, CC_MISC, sk_cooking, sk_null, 3, 1200);
  TOOL(itm_hotplate, 18, NULL);
  COMP(itm_meat_tainted, 1, NULL);
  COMP(itm_ammonia, 1, NULL);

 RECIPE(itm_laser_pack, CC_MISC, sk_electronics, sk_null, 5, 10000);
  TOOL(itm_screwdriver, -1, NULL);
  COMP(itm_superglue, 1, NULL);
  COMP(itm_plut_cell, 1, NULL);

 RECIPE(itm_bot_manhack, CC_MISC, sk_electronics, sk_computer, 6, 8000);
  TOOL(itm_screwdriver, -1, NULL);
  TOOL(itm_soldering_iron, 10, NULL);
  COMP(itm_knife_steak, 4, itm_knife_combat, 2, NULL);
  COMP(itm_processor, 1, NULL);
  COMP(itm_RAM, 1, NULL);
  COMP(itm_power_supply, 1, NULL);
  COMP(itm_battery, 200, itm_plut_cell, 1, NULL);

}

void game::craft()
{
 WINDOW *w_head = newwin( 3, 80, 0, 0);
 WINDOW *w_data = newwin(22, 80, 3, 0);
 craft_cat tab = CC_WEAPON;
 std::vector<recipe*> current;
 std::vector<bool> available;
 item tmp;
 int line = 0, xpos, ypos;
 bool redraw = true;
 bool done = false;
 char ch;

 do {
  if (redraw) { // When we switch tabs, redraw the header
   redraw = false;
   line = 0;
   draw_recipe_tabs(w_head, tab);
   current.clear();
   available.clear();
   pick_recipes(current, available, tab);
   werase(w_data);
   mvwprintz(w_data, 24, 0, c_white, "\
Press ? to describe object.  Press <ENTER> to attempt to craft object.");
   wrefresh(w_data);
  }

// Clear the screen of recipe data, and draw it anew
  werase(w_data);
  wrefresh(w_data);
  for (int i = 0; i < current.size() && i < 23; i++) {
   if (i == line)
    mvwprintz(w_data, i, 0, (available[i] ? h_white : h_dkgray),
              itypes[current[i]->result]->name.c_str());
   else
    mvwprintz(w_data, i, 0, (available[i] ? c_white : c_dkgray),
              itypes[current[i]->result]->name.c_str());
  }
  if (current.size() > 0) {
   nc_color col = (available[line] ? c_white : c_dkgray);
   mvwprintz(w_data, 0, 30, col, "Primary skill: %s",
             (current[line]->sk_primary == sk_null ? "N/A" :
              skill_name(current[line]->sk_primary).c_str()));
   mvwprintz(w_data, 1, 30, col, "Secondary skill: %s",
             (current[line]->sk_secondary == sk_null ? "N/A" :
              skill_name(current[line]->sk_secondary).c_str()));
   mvwprintz(w_data, 2, 30, col, "Difficulty: %d", current[line]->difficulty);
   if (current[line]->sk_primary == sk_null)
    mvwprintz(w_data, 3, 30, col, "Your skill level: N/A");
   else
    mvwprintz(w_data, 3, 30, col, "Your skill level: %d",
              u.sklevel[current[line]->sk_primary]);
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
    for (int i = 0; i < 5; i++) {
     if (current[line]->tools[i].size() > 0) {
      ypos++;
      mvwputch(w_data, ypos, 30, col, '>');
     }
     xpos = 32;
     for (int j = 0; j < current[line]->tools[i].size(); j++) {
      itype_id type = current[line]->tools[i][j].type;
      int charges = current[line]->tools[i][j].count;
      nc_color toolcol = c_red;
      if (charges < 0 && u.has_amount(type, 1))
       toolcol = c_green;
      else if (charges > 0 && u.has_charges(type, charges))
       toolcol = c_green;
      if (u.has_bionic(bio_tools))
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
     nc_color compcol = (u.has_number(type, count) ? c_green : c_red);
     std::stringstream dump;
     dump << count << "x " << itypes[type]->name << " ";
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
  ch = input();
  switch (ch) {
  case '<':
   if (tab == CC_WEAPON)
    tab = CC_MISC;
   else
    tab = craft_cat(int(tab) - 1);
   redraw = true;
   break;
  case '>':
   if (tab == CC_MISC)
    tab = CC_WEAPON;
   else
    tab = craft_cat(int(tab) + 1);
   redraw = true;
   break;
  case 'j':
   line++;
   break;
  case 'k':
   line--;
   break;
  case '\n':
   if (!available[line])
    popup("You can't do that!");
   else {
    make_craft(current[line]);
    done = true;
   }
   break;
  case '?':
   tmp = item(itypes[current[line]->result], 0);
   full_screen_popup(tmp.info(true).c_str());
   redraw = true;
   break;
  }
  if (line < 0)
   line = current.size() - 1;
  else if (line >= current.size())
   line = 0;
 } while (ch != KEY_ESCAPE && ch != 'q' && ch != 'Q' && !done);

 werase(w_head);
 werase(w_data);
 delwin(w_head);
 delwin(w_data);
 refresh_all();
}

void draw_recipe_tabs(WINDOW *w, craft_cat tab)
{
 werase(w);
 for (int i = 0; i < 80; i++) {
  mvwputch(w, 2, i, c_ltgray, LINE_OXOX);
  if ((i >  4 && i < 14) || (i > 20 && i < 27) || (i > 33 && i < 47) ||
      (i > 53 && i < 61) || (i > 67 && i < 74))
   mvwputch(w, 0, i, c_ltgray, LINE_OXOX);
 }


 mvwputch(w, 0,  4, c_ltgray, LINE_OXXO);
 mvwputch(w, 0, 20, c_ltgray, LINE_OXXO);
 mvwputch(w, 0, 33, c_ltgray, LINE_OXXO);
 mvwputch(w, 0, 53, c_ltgray, LINE_OXXO);
 mvwputch(w, 0, 67, c_ltgray, LINE_OXXO);
 mvwputch(w, 2,  4, c_ltgray, LINE_XXOX);
 mvwputch(w, 2, 20, c_ltgray, LINE_XXOX);
 mvwputch(w, 2, 33, c_ltgray, LINE_XXOX);
 mvwputch(w, 2, 53, c_ltgray, LINE_XXOX);
 mvwputch(w, 2, 67, c_ltgray, LINE_XXOX);

 mvwputch(w, 0, 14, c_ltgray, LINE_OOXX);
 mvwputch(w, 0, 27, c_ltgray, LINE_OOXX);
 mvwputch(w, 0, 47, c_ltgray, LINE_OOXX);
 mvwputch(w, 0, 61, c_ltgray, LINE_OOXX);
 mvwputch(w, 0, 74, c_ltgray, LINE_OOXX);
 mvwputch(w, 2, 14, c_ltgray, LINE_XXOX);
 mvwputch(w, 2, 27, c_ltgray, LINE_XXOX);
 mvwputch(w, 2, 47, c_ltgray, LINE_XXOX);
 mvwputch(w, 2, 61, c_ltgray, LINE_XXOX);
 mvwputch(w, 2, 74, c_ltgray, LINE_XXOX);

 mvwprintz(w, 1, 0, c_ltgray, "\
      WEAPONS         FOOD         ELECTRONICS         ARMOR         MISC");
 mvwputch(w, 1,  4, c_ltgray, LINE_XOXO);
 mvwputch(w, 1, 20, c_ltgray, LINE_XOXO);
 mvwputch(w, 1, 33, c_ltgray, LINE_XOXO);
 mvwputch(w, 1, 53, c_ltgray, LINE_XOXO);
 mvwputch(w, 1, 67, c_ltgray, LINE_XOXO);
 mvwputch(w, 1, 14, c_ltgray, LINE_XOXO);
 mvwputch(w, 1, 27, c_ltgray, LINE_XOXO);
 mvwputch(w, 1, 47, c_ltgray, LINE_XOXO);
 mvwputch(w, 1, 61, c_ltgray, LINE_XOXO);
 mvwputch(w, 1, 74, c_ltgray, LINE_XOXO);

 switch (tab) {
 case CC_WEAPON:
  for (int i = 5; i < 14; i++)
   mvwputch(w, 2, i, c_black, ' ');
  mvwprintz(w, 1, 6, h_ltgray, "WEAPONS");
  mvwputch(w, 2,  4, c_ltgray, LINE_XOOX);
  mvwputch(w, 2, 14, c_ltgray, LINE_XXOO);
  mvwputch(w, 1,  2, h_ltgray, '<');
  mvwputch(w, 1, 16, h_ltgray, '>');
  break;
 case CC_FOOD:
  for (int i = 21; i < 27; i++)
   mvwputch(w, 2, i, c_black, ' ');
  mvwprintz(w, 1, 22, h_ltgray, "FOOD");
  mvwputch(w, 2, 20, c_ltgray, LINE_XOOX);
  mvwputch(w, 2, 27, c_ltgray, LINE_XXOO);
  mvwputch(w, 1, 18, h_ltgray, '<');
  mvwputch(w, 1, 29, h_ltgray, '>');
  break;
 case CC_ELECTRONIC:
  for (int i = 34; i < 47; i++)
   mvwputch(w, 2, i, c_black, ' ');
  mvwprintz(w, 1, 35, h_ltgray, "ELECTRONICS");
  mvwputch(w, 2, 33, c_ltgray, LINE_XOOX);
  mvwputch(w, 2, 47, c_ltgray, LINE_XXOO);
  mvwputch(w, 1, 31, h_ltgray, '<');
  mvwputch(w, 1, 49, h_ltgray, '>');
  break;
 case CC_ARMOR:
  for (int i = 54; i < 61; i++)
   mvwputch(w, 2, i, c_black, ' ');
  mvwprintz(w, 1, 55, h_ltgray, "ARMOR");
  mvwputch(w, 2, 53, c_ltgray, LINE_XOOX);
  mvwputch(w, 2, 61, c_ltgray, LINE_XXOO);
  mvwputch(w, 1, 51, h_ltgray, '<');
  mvwputch(w, 1, 63, h_ltgray, '>');
  break;
 case CC_MISC:
  for (int i = 68; i < 74; i++)
   mvwputch(w, 2, i, c_black, ' ');
  mvwprintz(w, 1, 69, h_ltgray, "MISC");
  mvwputch(w, 2, 67, c_ltgray, LINE_XOOX);
  mvwputch(w, 2, 74, c_ltgray, LINE_XXOO);
  mvwputch(w, 1, 65, h_ltgray, '<');
  mvwputch(w, 1, 76, h_ltgray, '>');
  break;
 }
 wrefresh(w);
}

void game::pick_recipes(std::vector<recipe*> &current,
                        std::vector<bool> &available, craft_cat tab)
{
 bool have_tool[5], have_comp[5];
 current.clear();
 available.clear();
 for (int i = 0; i < recipes.size(); i++) {
// Check if the category matches the tab, and we have the requisite skills
  if (recipes[i].category == tab &&
      (recipes[i].sk_primary == sk_null ||
       u.sklevel[recipes[i].sk_primary] >= recipes[i].difficulty) &&
      (recipes[i].sk_secondary == sk_null ||
       u.sklevel[recipes[i].sk_secondary] > 0))
  current.push_back(&(recipes[i]));
  available.push_back(false);
 }
 for (int i = 0; i < current.size() && i < 22; i++) {
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
     if (u.has_bionic(bio_tools) ||
         (req < 0 && u.has_amount(type, 1)) ||
         (req > 0 && u.has_charges(type, req))) {
      have_tool[j] = true;
      k = current[i]->tools[j].size();
     }
    }
   }
   if (current[i]->components[j].size() == 0)
    have_comp[j] = true;
   else {
    for (int k = 0; k < current[i]->components[j].size(); k++) {
     itype_id type = current[i]->components[j][k].type;
     int count = current[i]->components[j][k].count;
     if (u.has_number(type, count)) {
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
 u.activity = player_activity(ACT_CRAFT, making->time, making->id);
 u.moves = 0;
}

void game::complete_craft()
{
 std::vector<component> will_use;
 recipe making = recipes[u.activity.index];
 for (int i = 0; i < 5; i++) {
  if (making.components[i].size() > 0) {
   std::vector<component> you_have;
   for (int j = 0; j < making.components[i].size(); j++) {
    if (u.has_number(making.components[i][j].type,
                     making.components[i][j].count))
     you_have.push_back(making.components[i][j]);
   }
   if (you_have.size() == 1)
    will_use.push_back(component(you_have[0].type, you_have[0].count));
   else {	// Let the player pick which component they want to use
    WINDOW* w = newwin(you_have.size() + 2, 30, 10, 25);
    wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
               LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
    mvwprintz(w, 0, 5, c_red, "Use which component?");
    for (int j = 0; j < you_have.size(); j++)
     mvwprintz(w, j + 1, 1, c_white, "%d: %s", j + 1,
               itypes[you_have[j].type]->name.c_str());
    wrefresh(w);
    char ch;
    do
     ch = getch();
    while (ch < '1' || ch >= '1' + you_have.size());
    ch -= '1';
    will_use.push_back(component(you_have[ch].type, you_have[ch].count));
   }
  }
  if (making.tools[i].size() > 0) {
   for (int j = 0; j < making.tools[i].size(); j++) {
    if (making.tools[i][j].count > 0)
     u.use_charges(making.tools[i][j].type, making.tools[i][j].count);
   }
  }
 }
 int skill_roll = (making.difficulty == 0 ? 1 :
                   dice(u.sklevel[making.sk_primary] * 3 + 
                        u.sklevel[making.sk_secondary], 20 + u.int_cur));
 int diff_roll = (making.difficulty == 0 ? 0 :
                  dice(making.difficulty * 4, 28));
 if (making.sk_primary != sk_null)
  u.practice(making.sk_primary, making.difficulty * 4 + 12);
 if (making.sk_secondary != sk_null)
  u.practice(making.sk_secondary, 5);
 if (diff_roll >= skill_roll * (1 + 0.1 * rng(1, 5))) {
  add_msg("You fail to make the %s, and waste some materials.",
          itypes[making.result]->name.c_str());
  int num_lost = rng(1, will_use.size());
  for (int i = 0; i < num_lost; i++) {
   int n = rng(0, will_use.size() - 1);
   if (itypes[will_use[n].type]->is_ammo())
    u.use_charges(will_use[n].type, will_use[n].count);
   else
    u.use_amount(will_use[n].type, will_use[n].count);
   will_use.erase(will_use.begin() + n);
  }
  u.activity.type = ACT_NULL;
  return;
 } else if (diff_roll > skill_roll) {
  add_msg("You fail to make the %s, but don't waste any materials.",
          itypes[making.result]->name.c_str());
  u.activity.type = ACT_NULL;
  return;
 }
 for (int i = 0; i < will_use.size(); i++) {
  if (itypes[will_use[i].type]->is_ammo())
   u.use_charges(will_use[i].type, will_use[i].count);
   else
   u.use_amount(will_use[i].type, will_use[i].count);
 }
 int iter = 0;
 item newit(itypes[making.result], turn, nextinv);
 do {
  newit.invlet = nextinv;
  advance_nextinv();
  iter++;
 } while (u.has_item(newit.invlet) && iter < 52);
 newit = newit.in_its_container(&itypes);
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
