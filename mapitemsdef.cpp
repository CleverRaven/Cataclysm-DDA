#include "game.h"
#include "itype.h"
#include "omdata.h"
#include "setvector.h"
#include <cstdarg>

void game::init_mapitems()
{
 setvector(
   mapitems[mi_field],
	itm_rock, itm_strawberries, NULL);
 setvector(
   mapitems[mi_forest],
	itm_rock, itm_stick, itm_mushroom, itm_mushroom_poison,
	itm_mushroom_magic, itm_blueberries, NULL);
 setvector(
   mapitems[mi_hive],
	itm_honeycomb, NULL);
 setvector(
   mapitems[mi_hive_center],
	itm_honeycomb, itm_royal_jelly, NULL);
 setvector(
   mapitems[mi_road],
	itm_muffler, itm_pipe, itm_motor, NULL);
 setvector(
   mapitems[mi_livingroom],
	itm_rootbeer, itm_cola, itm_cig, itm_weed, itm_coke, itm_meth,
	itm_sneakers, itm_boots, itm_boots_winter, itm_flip_flops,
	itm_dress_shoes, itm_heels, itm_coat_rain, itm_poncho, itm_gloves_light,
	itm_mittens, itm_gloves_light, itm_mittens, itm_gloves_wool,
	itm_gloves_winter, itm_gloves_leather, itm_gloves_fingerless,
	itm_bandana, itm_scarf, itm_hat_cotton, itm_hat_knit, itm_hat_fur,
	itm_helmet_bike, itm_helmet_motor, itm_mag_tv, itm_mag_news,
	itm_lighter, itm_extinguisher, NULL);
 setvector(
   mapitems[mi_kitchen],
	itm_pot, itm_pan, itm_knife_butter, itm_knife_steak, itm_knife_butcher,
	itm_cookbook, itm_rag, itm_hotplate, itm_flashlight, itm_extinguisher,
	itm_whiskey, itm_bleach, itm_ammonia,
	NULL);
 setvector(
   mapitems[mi_fridge],
	itm_water, itm_oj, itm_cola, itm_rootbeer, itm_milk, itm_V8, itm_apple,
	itm_sandwich_t, itm_mushroom, itm_blueberries, itm_strawberries, 
	itm_tomato, itm_broccoli, itm_zucchini, itm_frozen_dinner, itm_vodka,
	NULL);
 setvector(
   mapitems[mi_bedroom],
	itm_inhaler, itm_cig, itm_weed, itm_coke, itm_meth, itm_heroin,
	itm_sneakers, itm_mocassins, itm_bandana, itm_glasses_eye,
	itm_glasses_reading, itm_hat_ball, itm_backpack, itm_purse, itm_mbag,
	itm_fanny, itm_battery, itm_bb, itm_bbgun, itm_mag_porn, itm_mag_tv,
	itm_mag_news, itm_novel_romance, itm_novel_drama, itm_manual_mechanics,
	itm_manual_speech, itm_manual_business, itm_manual_computers,
	itm_lighter, itm_sewing_kit, itm_scissors, itm_soldering_iron,
	itm_radio, itm_syringe, NULL);
 setvector(
   mapitems[mi_homeguns],
	itm_22_lr, itm_9mm, itm_crossbow, itm_sig_mosquito, itm_sw_22,
	itm_glock_19, itm_usp_9mm, itm_sw_619, itm_taurus_38, itm_sig_40,
	itm_sw_610, itm_ruger_redhawk, itm_deagle_44, itm_usp_45, itm_m1911,
	itm_fn57, itm_mac_10, itm_shotgun_sawn, itm_silencer, itm_grip,
	itm_clip, itm_grenade, itm_gasbomb, NULL);
 setvector(
   mapitems[mi_dresser],
	itm_jeans, itm_pants, itm_pants_leather, itm_pants_cargo, itm_skirt,
	itm_dress, itm_tshirt, itm_polo_shirt, itm_dress_shirt, itm_tank_top,
	itm_sweatshirt, itm_sweater, itm_hoodie, itm_jacket_light,
	itm_jacket_jean, itm_blazer, itm_jacket_leather, itm_poncho,
	itm_trenchcoat, itm_peacoat, itm_vest, itm_mag_porn, itm_lighter,
	itm_sewing_kit, itm_flashlight, NULL);
 setvector(
   mapitems[mi_snacks],
	itm_chips, itm_pretzels, itm_chocolate, itm_jerky, itm_candy, NULL);
 setvector(
   mapitems[mi_fridgesnacks],
	itm_water, itm_oj, itm_energy_drink, itm_cola, itm_rootbeer, itm_milk,
	itm_V8, itm_sandwich_t, itm_frozen_dinner, NULL);
 setvector(
   mapitems[mi_behindcounter],
	itm_aspirin, itm_caffeine, itm_cig, itm_battery, itm_shotgun_sawn,
	itm_mag_porn, itm_lighter, itm_flashlight, itm_extinguisher, NULL);
 setvector(
   mapitems[mi_magazines],
	itm_mag_tv, itm_mag_news, itm_mag_cars, itm_mag_cooking,
	itm_novel_romance, itm_novel_spy, NULL);
 setvector(
   mapitems[mi_softdrugs],
	itm_bandages, itm_1st_aid, itm_vitamins, itm_aspirin, itm_caffeine,
	itm_pills_sleep, itm_dayquil, itm_nyquil, NULL);
 setvector(
   mapitems[mi_harddrugs],
	itm_inhaler, itm_codeine, itm_oxycodone, itm_tramadol, itm_xanax,
	itm_adderall, itm_thorazine, itm_prozac, itm_syringe, NULL);
 setvector(
   mapitems[mi_cannedfood],
	itm_can_beans, itm_can_corn, itm_can_spam, itm_can_pineapple,
	itm_can_coconut, itm_can_sardine, itm_can_tuna, itm_can_catfood, NULL);
 setvector(
   mapitems[mi_pasta],
	itm_spaghetti_raw, itm_macaroni_raw, itm_ravioli, itm_sauce_red,
	itm_sauce_pesto, NULL);
 setvector(
   mapitems[mi_produce],
	itm_apple, itm_orange, itm_mushroom, itm_blueberries, itm_strawberries,
	itm_tomato, itm_broccoli, itm_zucchini, NULL);
 setvector(
   mapitems[mi_cleaning],
	itm_salt_water, itm_bleach, itm_ammonia, itm_broom, itm_mop,
	itm_gloves_rubber, itm_mask_dust, itm_bottle_plastic, itm_sewing_kit,
	itm_rag, itm_scissors, NULL);
 setvector(
   mapitems[mi_hardware],
	itm_superglue, itm_chain, itm_rope_6, itm_rope_30, itm_glass_sheet,
	itm_pipe, itm_nail, itm_hose, NULL);
 setvector(
   mapitems[mi_tools],
	itm_screwdriver, itm_hammer, itm_wrench, itm_saw, itm_hacksaw,
	itm_hammer_sledge, itm_xacto, itm_flashlight, itm_crowbar, itm_nailgun,
	NULL);
 setvector(
   mapitems[mi_bigtools],
	itm_broom, itm_mop, itm_hoe, itm_shovel, itm_chainsaw_off,
	itm_jackhammer, NULL);
 setvector(
   mapitems[mi_mischw],
	itm_2x4, itm_machete, itm_boots_steel, itm_hat_hard, itm_mask_filter,
	itm_glasses_safety, itm_bb, itm_bbgun, itm_beartrap, itm_two_way_radio,
	itm_radio, itm_hotplate, itm_extinguisher, itm_nailgun, NULL);
 setvector(
   mapitems[mi_sports],
	itm_bandages, itm_aspirin, itm_bat, itm_sneakers, itm_tshirt,
	itm_tank_top, itm_gloves_fingerless, itm_glasses_safety,
	itm_goggles_swim, itm_goggles_ski, itm_hat_ball, itm_helmet_bike,
	itm_helmet_ball, itm_manual_brawl, NULL);
 setvector(
   mapitems[mi_camping],
	itm_rope_30, itm_hatchet, itm_pot, itm_pan, itm_hotplate,
	itm_knife_combat, itm_machete, itm_vest, itm_backpack, itm_bb,
	itm_bolt_steel, itm_bbgun, itm_crossbow, itm_manual_knives,
	itm_manual_first_aid, itm_lighter, itm_sewing_kit, itm_hammer,
	itm_flashlight, itm_water_purifier, itm_radio, itm_beartrap, NULL);
 setvector(
   mapitems[mi_allsporting],
	itm_aspirin, itm_bat, itm_sneakers, itm_tshirt, itm_tank_top,
	itm_gloves_fingerless, itm_glasses_safety, itm_goggles_swim,
	itm_goggles_ski, itm_hat_ball, itm_helmet_bike, itm_helmet_ball,
	itm_manual_brawl, itm_rope_30, itm_hatchet, itm_pot, itm_pan,
	itm_hotplate, itm_knife_combat, itm_machete, itm_vest, itm_backpack,
	itm_bb, itm_bolt_steel, itm_bbgun, itm_crossbow, itm_manual_knives,
	itm_manual_first_aid, itm_lighter, itm_sewing_kit, itm_hammer,
	itm_flashlight, itm_water_purifier, itm_radio, itm_beartrap,
	itm_extinguisher, NULL);
 setvector(
   mapitems[mi_alcohol],
	itm_whiskey, itm_vodka, itm_rum, itm_tequila, NULL);
 setvector(
   mapitems[mi_trash],
	itm_meth, itm_heroin, itm_wrapper, itm_string_6, itm_chain,
	itm_glass_sheet, itm_stick, itm_muffler, itm_pipe, itm_bag_plastic,
	itm_bottle_plastic, itm_bottle_glass, itm_can_drink, itm_can_food,
	itm_box_small, itm_bubblewrap, itm_lighter, itm_syringe, itm_rag, NULL);
 setvector(
   mapitems[mi_ammo],
	itm_shot_bird, itm_shot_00, itm_shot_slug, itm_22_lr, itm_22_cb,
	itm_9mm, itm_9mmP, itm_9mmP2, itm_38_special, itm_38_super, itm_10mm,
	itm_40sw, itm_44magnum, itm_45_acp, itm_45_jhp, itm_45_super, itm_57mm,
	itm_46mm, itm_762_m43, itm_762_m87, itm_223, itm_556, itm_270, itm_3006,
	itm_308, itm_762_51, NULL);
 setvector(
   mapitems[mi_pistols],
	itm_sig_mosquito, itm_sw_22, itm_glock_19, itm_usp_9mm, itm_sw_619,
	itm_taurus_38, itm_sig_40, itm_sw_610, itm_ruger_redhawk, itm_deagle_44,
	itm_usp_45, itm_m1911, itm_fn57, itm_hk_ucp, NULL);
 setvector(
   mapitems[mi_shotguns],
	itm_shotgun_s, itm_shotgun_d, itm_remington_870, itm_mossberg_500,
	itm_saiga_12, NULL);
 setvector(
   mapitems[mi_rifles],
	itm_marlin_9a, itm_ruger_1022, itm_browning_blr, itm_remington_700,
	itm_sks, itm_ruger_mini, itm_savage_111f, NULL);
 setvector(
   mapitems[mi_smg],
	itm_uzi, itm_tec9, itm_calico, itm_hk_mp5, itm_mac_10, itm_hk_ump45,
	itm_TDI, itm_fn_p90, itm_hk_mp7, NULL);
 setvector(
   mapitems[mi_assault],
	itm_hk_g3, itm_hk_g36, itm_ak47, itm_fn_fal, itm_acr, itm_ar15,
	itm_scar_l, itm_scar_h, itm_m249, NULL);
 setvector(
   mapitems[mi_allguns],
	itm_sig_mosquito, itm_sw_22, itm_glock_19, itm_usp_9mm, itm_sw_619,
	itm_taurus_38, itm_sig_40, itm_sw_610, itm_ruger_redhawk, itm_deagle_44,
	itm_usp_45, itm_m1911, itm_fn57, itm_hk_ucp, itm_shotgun_s,
	itm_shotgun_d, itm_remington_870, itm_mossberg_500, itm_saiga_12,
	itm_uzi, itm_tec9, itm_calico, itm_hk_mp5, itm_mac_10, itm_hk_ump45,
	itm_TDI, itm_fn_p90, itm_hk_mp7, itm_marlin_9a, itm_ruger_1022,
	itm_browning_blr, itm_remington_700, itm_sks, itm_ruger_mini,
	itm_savage_111f, itm_hk_g3, itm_hk_g36, itm_ak47, itm_fn_fal, itm_acr,
	itm_ar15, itm_scar_l, itm_scar_h, itm_m249, NULL);
 setvector(
   mapitems[mi_gunxtras],
	itm_glasses_safety, itm_goggles_nv, itm_holster, itm_bootstrap,
	itm_mag_guns, itm_flashlight, itm_silencer, itm_grip, itm_barrel_big,
	itm_barrel_small, itm_barrel_rifled, itm_clip, itm_clip2, itm_stablizer,
	itm_blowback, itm_autofire, itm_retool_45, itm_retool_9mm,
	itm_retool_22, itm_retool_57, itm_retool_46, itm_retool_308,
	itm_retool_223, NULL);
 setvector(
   mapitems[mi_shoes],
	itm_sneakers, itm_boots, itm_flip_flops, itm_dress_shoes, itm_heels, 
	NULL);
 setvector(
   mapitems[mi_pants],
	itm_jeans, itm_pants, itm_pants_leather, itm_pants_cargo, itm_skirt,
	itm_dress, NULL);
 setvector(
   mapitems[mi_shirts],
	itm_tshirt, itm_polo_shirt, itm_dress_shirt, itm_tank_top,
	itm_sweatshirt, itm_sweater, itm_hoodie, NULL);
 setvector(
   mapitems[mi_jackets],
	itm_jacket_light, itm_jacket_jean, itm_blazer, itm_jacket_leather,
	itm_coat_rain, itm_trenchcoat, NULL);
 setvector(
   mapitems[mi_winter],
	itm_coat_winter, itm_peacoat, itm_gloves_light, itm_mittens,
	itm_gloves_wool, itm_gloves_winter, itm_gloves_leather, itm_scarf,
	itm_hat_cotton, itm_hat_knit, itm_hat_fur, NULL);
 setvector(
   mapitems[mi_bags],
	itm_backpack, itm_purse, itm_mbag, NULL);
 setvector(
   mapitems[mi_allclothes],
	itm_jeans, itm_pants, itm_pants_leather, itm_pants_cargo, itm_skirt,
	itm_tshirt, itm_polo_shirt, itm_dress_shirt, itm_tank_top,
	itm_sweatshirt, itm_sweater, itm_hoodie, itm_jacket_light,
	itm_jacket_jean, itm_blazer, itm_jacket_leather, itm_coat_winter,
	itm_peacoat, itm_gloves_light, itm_mittens, itm_gloves_wool,
	itm_gloves_winter, itm_gloves_leather, itm_scarf, itm_hat_cotton,
	itm_hat_knit, itm_hat_fur, NULL);
 setvector(
   mapitems[mi_novels],
	itm_novel_romance, itm_novel_spy, itm_novel_scifi, itm_novel_drama,
	NULL);
 setvector(
   mapitems[mi_manuals],
	itm_manual_brawl, itm_manual_knives, itm_manual_mechanics,
	itm_manual_speech, itm_manual_business, itm_manual_first_aid,
	itm_manual_computers, itm_cookbook, itm_manual_electronics,
	itm_manual_tailor, NULL);
 setvector(
   mapitems[mi_textbooks],
	itm_textbook_computers, itm_textbook_electronics, itm_textbook_business,
	itm_textbook_chemistry, itm_SICP, itm_textbook_robots, NULL);
 setvector(
   mapitems[mi_chemistry],
	itm_water, itm_salt_water, itm_bleach, itm_ammonia, itm_mutagen,
	itm_purifier, itm_royal_jelly, itm_superglue, itm_bottle_glass,
	itm_syringe, itm_rag, itm_extinguisher, itm_hotplate, NULL);
 setvector(
   mapitems[mi_teleport],
	itm_screwdriver, itm_wrench, itm_jumpsuit, itm_mask_dust,
	itm_glasses_safety, itm_goggles_welding, itm_teleporter, NULL);
 setvector(
   mapitems[mi_goo],
	itm_jumpsuit, itm_gloves_rubber, itm_mask_filter, itm_glasses_safety,
	itm_helmet_riot, itm_lighter, itm_canister_goo, NULL);
 setvector(
   mapitems[mi_cloning_vat],
	itm_fetus, itm_arm, itm_leg, NULL);
 setvector(
   mapitems[mi_dissection],
	itm_bleach, itm_bandages, itm_string_6, itm_hacksaw, itm_xacto,
	itm_knife_butcher, itm_machete, itm_gloves_rubber, itm_bag_plastic,
	itm_syringe, itm_rag, itm_scissors, NULL);
 setvector(
   mapitems[mi_hydro],
	itm_blueberries, itm_strawberries, itm_tomato, itm_broccoli,
	itm_zucchini, NULL);
 setvector(
   mapitems[mi_electronics],
	itm_superglue, itm_electrohack, itm_processor, itm_RAM,
	itm_power_supply, itm_amplifier, itm_transponder, itm_receiver,
	itm_antenna, itm_motor, itm_screwdriver, itm_mask_dust,
	itm_glasses_safety, itm_goggles_welding, itm_battery, itm_plut_cell,
	itm_manual_electronics, itm_textbook_electronics, itm_soldering_iron,
	itm_hotplate, NULL);
 setvector(
   mapitems[mi_monparts],
	itm_meat, itm_veggy, itm_meat_tainted, itm_veggy_tainted,
	itm_royal_jelly, itm_ant_egg, itm_bee_sting, itm_chitin_piece, NULL);
 setvector(
   mapitems[mi_bionics],
	itm_bionics_power, itm_bionics_tools, itm_bionics_neuro,
	itm_bionics_sensory, itm_bionics_aquatic, itm_bionics_combat_aug,
	itm_bionics_hazmat, itm_bionics_nutritional, itm_bionics_desert,
	itm_bionics_melee, itm_bionics_armor, itm_bionics_espionage,
	itm_bionics_defense, itm_bionics_medical, itm_bionics_construction,
	itm_bionics_super, itm_bionics_ranged, NULL);
 setvector(
   mapitems[mi_weapons],
	itm_chain, itm_hammer, itm_wrench, itm_hammer_sledge, itm_hatchet,
	itm_ax, itm_knife_combat, itm_pipe, itm_bat, itm_machete, itm_katana,
	itm_baton, NULL);
 setvector(
   mapitems[mi_survival_armor],
	itm_boots_steel, itm_pants_cargo, itm_pants_army, itm_jumpsuit,
	itm_jacket_leather, itm_kevlar, itm_vest, itm_gloves_fingerless,
	itm_mask_filter, itm_mask_gas, itm_goggles_ski, itm_helmet_skid,
	itm_helmet_ball, itm_helmet_riot, itm_helmet_motor, itm_holster,
	itm_bootstrap, NULL);
 setvector(
   mapitems[mi_survival_tools],
	itm_bandages, itm_1st_aid, itm_caffeine, itm_electrohack, itm_string_36,
	itm_rope_30, itm_chain, itm_bottle_plastic, itm_lighter, itm_sewing_kit,
	itm_extinguisher, itm_flashlight, itm_crowbar, itm_chainsaw_off,
	itm_beartrap, itm_grenade, itm_hotplate, NULL);
 setvector(
   mapitems[mi_radio],
	itm_cola, itm_caffeine, itm_cig, itm_weed, itm_amplifier,
	itm_transponder, itm_receiver, itm_antenna, itm_screwdriver,
	itm_battery, itm_mag_porn, itm_mag_tv, itm_manual_electronics,
	itm_lighter, itm_flashlight, itm_two_way_radio, itm_radio, NULL);
 setvector(
   mapitems[mi_subway],
	itm_wrapper, itm_string_6, itm_chain, itm_rock, itm_pipe,
	itm_mag_porn, itm_bottle_plastic, itm_bottle_glass, itm_can_drink,
	itm_can_food, itm_lighter, itm_flashlight, itm_rag, itm_crowbar, NULL);
 setvector(
   mapitems[mi_sewer],
	itm_mutagen, itm_fetus, itm_weed, itm_mag_porn, itm_rag, NULL);
 setvector(
   mapitems[mi_cavern],
	itm_rock, itm_jackhammer, NULL);
 setvector(
   mapitems[mi_ant_food],
	itm_meat, itm_veggy, itm_meat_tainted, itm_veggy_tainted, itm_apple,
	itm_orange, itm_mushroom, itm_blueberries, itm_strawberries,
	itm_tomato, itm_broccoli, itm_zucchini, itm_honeycomb, itm_royal_jelly,
	itm_arm, itm_leg, itm_rock, itm_stick, NULL);
 setvector(
   mapitems[mi_ant_egg],
	itm_ant_egg, NULL);	//TODO: More items here?

 setvector(
   mapitems[mi_ants],
	itm_chitin_piece, NULL);

 setvector(
   mapitems[mi_bees],
	itm_bee_sting, itm_chitin_piece, NULL);
 
 setvector(
   mapitems[mi_robots],
	itm_processor, itm_RAM, itm_power_supply, itm_amplifier,
	itm_transponder, itm_receiver, itm_antenna, itm_steel_chunk, itm_motor,
	itm_battery, itm_plut_cell, NULL);

// This one kind of an inverted list; what a traveling salesman will NOT carry
 setvector(
   mapitems[mi_trader_avoid],
	itm_water_dirty, itm_meat, itm_veggy, itm_meat_tainted,
	itm_veggy_tainted, itm_meat_cooked, itm_veggy_cooked,
	itm_mushroom_poison, itm_spaghetti_cooked, itm_macaroni_cooked,
	itm_fetus, itm_arm, itm_leg, itm_wrapper, itm_manhole_cover, itm_rock,
	itm_stick, itm_bag_plastic, itm_flashlight_on, itm_radio_on,
	itm_chainsaw_on, itm_grenade_act, itm_gasbomb_act, itm_molotov_lit,
	itm_dynamite_act, itm_mininuke_act, NULL);
}
