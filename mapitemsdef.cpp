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
	itm_muffler, itm_pipe, itm_motor, itm_wheel, itm_big_wheel, itm_seat, 
        itm_combustion_tiny, itm_combustion_small, itm_combustion, itm_foot_crank, NULL);

 setvector(
   mapitems[mi_livingroom],
	itm_rootbeer, itm_pizza, itm_cola, itm_cig, itm_cigar, itm_weed,
	itm_coke, itm_meth, itm_sneakers, itm_boots, itm_boots_winter,
	itm_flip_flops, itm_dress_shoes, itm_heels, itm_coat_rain, itm_poncho,
	itm_gloves_light, itm_mittens, itm_gloves_light, itm_mittens,
	itm_armguard_soft, itm_armguard_hard,
	itm_gloves_wool, itm_gloves_winter, itm_gloves_leather,
	itm_gloves_fingerless, itm_bandana, itm_scarf, itm_hat_cotton,
	itm_hat_knit, itm_hat_fur, itm_helmet_bike, itm_helmet_motor,
	itm_mag_tv, itm_mag_news, itm_lighter, itm_extinguisher, itm_mp3, 
	itm_usb_drive, itm_sneakers_fit, itm_boots_fit, itm_boots_steel_fit, 
        itm_boots_winter_fit, itm_dress_shoes_fit, itm_heels_fit, NULL);

 setvector(
   mapitems[mi_kitchen],
	itm_pot, itm_pan, itm_knife_butter, itm_knife_steak, itm_knife_butcher,
	itm_cookbook, itm_rag, itm_hotplate, itm_flashlight, itm_extinguisher,
	itm_whiskey, itm_bleach, itm_ammonia, itm_flour, itm_sugar, itm_salt,
	itm_tea_raw, itm_coffee_raw, 
	NULL);

 setvector(
   mapitems[mi_fridge],
	itm_water_clean, itm_oj, itm_cola, itm_rootbeer, itm_milk, itm_V8, itm_apple,
	itm_sandwich_t, itm_mushroom, itm_blueberries, itm_strawberries, 
	itm_tomato, itm_broccoli, itm_zucchini, itm_frozen_dinner, itm_vodka,
	itm_apple_cider, NULL);

 setvector(
   mapitems[mi_home_hw],
	itm_superglue, itm_string_6, itm_string_36, itm_screwdriver, itm_wrench,
	itm_hacksaw, itm_xacto, itm_gloves_leather, itm_mask_dust,
	itm_glasses_safety, itm_battery, itm_nail, itm_nailgun,
	itm_manual_mechanics, itm_hammer, itm_flashlight, itm_soldering_iron,
	itm_bubblewrap, itm_binoculars, itm_duct_tape, itm_lawnmower, itm_foot_crank,
        itm_boltcutters, NULL);

 setvector(
   mapitems[mi_bedroom],
	itm_inhaler, itm_cig, itm_cigar, itm_weed, itm_coke, itm_meth,
	itm_heroin, itm_sneakers, itm_mocassins, itm_bandana, itm_glasses_eye, itm_sunglasses,
	itm_glasses_reading, itm_hat_ball, itm_backpack, itm_purse, itm_mbag,
	itm_fanny, itm_battery, itm_bb, itm_bbgun, itm_mag_porn, itm_mag_tv,
	itm_mag_news, itm_novel_romance, itm_novel_drama, itm_manual_mechanics,
	itm_manual_speech, itm_manual_business, itm_manual_computers,
	itm_lighter, itm_sewing_kit, itm_thread, itm_scissors, itm_soldering_iron,
	itm_radio, itm_syringe, itm_mp3, itm_usb_drive,
        itm_sneakers_fit, itm_boots_fit, itm_boots_steel_fit, itm_boots_winter_fit,
        itm_dress_shoes_fit, itm_heels_fit, itm_chips, itm_cola, itm_picklocks, NULL);

 setvector(
   mapitems[mi_homeguns],
	itm_22_lr, itm_9mm, itm_crossbow, itm_sig_mosquito, itm_sw_22,
	itm_glock_19, itm_usp_9mm, itm_sw_619, itm_taurus_38, itm_sig_40,
	itm_sw_610, itm_ruger_redhawk, itm_deagle_44, itm_usp_45, itm_m1911,
	itm_fn57, itm_mac_10, itm_shotgun_sawn, itm_silencer, itm_grip,
	itm_clip, itm_spare_mag, itm_grenade, itm_EMPbomb, itm_gasbomb, itm_tazer,
	itm_longbow, itm_compbow, itm_arrow_wood, itm_arrow_cf, itm_u_shotgun, itm_shot_hull,
        itm_9mm_casing, itm_38_casing, itm_40_casing, itm_44_casing, itm_45_casing,
        itm_57mm_casing, itm_46mm_casing, itm_762_casing, itm_223_casing,
        itm_3006_casing, itm_308_casing, itm_gunpowder,
        itm_shotgun_primer, itm_smpistol_primer, itm_lgpistol_primer,
        itm_smrifle_primer, itm_lgrifle_primer, itm_lead, NULL);

 setvector(
   mapitems[mi_dresser],
	itm_jeans, itm_pants, itm_pants_leather, itm_pants_cargo, itm_skirt,
	itm_dress, itm_tshirt, itm_polo_shirt, itm_dress_shirt, itm_tank_top,
	itm_sweatshirt, itm_sweater, itm_hoodie, itm_jacket_light,
	itm_jacket_jean, itm_blazer, itm_jacket_leather, itm_poncho,
	itm_trenchcoat, itm_peacoat, itm_vest, itm_mag_porn, itm_lighter,
	itm_sewing_kit, itm_thread, itm_flashlight, itm_suit, itm_tophat,
	itm_glasses_monocle, itm_duct_tape, 
        itm_jeans_fit, itm_pants_fit, itm_pants_cargo_fit,
        itm_polo_shirt_fit, itm_sweatshirt_fit, itm_sweater_fit,
        itm_jacket_light_fit, itm_tshirt_fit, itm_trenchcoat_fit,

        NULL);

 setvector(
   mapitems[mi_dining],
	itm_wrapper, itm_knife_butter, itm_knife_steak, itm_bottle_glass,
	NULL);

 setvector(
   mapitems[mi_snacks],
	itm_chips, itm_pretzels, itm_chocolate, itm_jerky, itm_candy,
	itm_tea_raw, itm_coffee_raw, itm_chips2, itm_chips3, NULL);

 setvector(
   mapitems[mi_fridgesnacks],
	itm_water_clean, itm_oj, itm_apple_cider, itm_energy_drink, itm_cola,
	itm_rootbeer, itm_milk, itm_V8, itm_sandwich_t, itm_frozen_dinner,
	itm_pizza, itm_pie, NULL);

 setvector(
   mapitems[mi_behindcounter],
	itm_aspirin, itm_caffeine, itm_cig, itm_cigar, itm_battery,
	itm_shotgun_sawn, itm_mag_porn, itm_lighter, itm_flashlight,
	itm_extinguisher, itm_tazer, itm_mp3, NULL);

 setvector(
   mapitems[mi_magazines],
	itm_mag_tv, itm_mag_news, itm_mag_cars, itm_mag_cooking,
	itm_novel_romance, itm_novel_spy, itm_mag_carpentry,  NULL);

 setvector(
   mapitems[mi_softdrugs],
	itm_bandages, itm_1st_aid, itm_vitamins, itm_aspirin, itm_caffeine,
	itm_pills_sleep, itm_iodine, itm_dayquil, itm_nyquil, NULL);

 setvector(
   mapitems[mi_harddrugs],
	itm_inhaler, itm_codeine, itm_oxycodone, itm_tramadol, itm_xanax,
	itm_adderall, itm_thorazine, itm_prozac, itm_antibiotics, itm_syringe,
	 NULL);

 setvector(
   mapitems[mi_cannedfood],
	itm_can_beans, itm_can_corn, itm_can_spam, itm_can_pineapple,
	itm_can_coconut, itm_can_sardine, itm_can_tuna, itm_can_catfood,
	itm_broth, itm_soup, itm_flour, itm_sugar, itm_salt, NULL);

 setvector(
   mapitems[mi_pasta],
	itm_spaghetti_raw, itm_macaroni_raw, itm_ravioli, itm_sauce_red,
	itm_sauce_pesto, itm_bread, NULL);

 setvector(
   mapitems[mi_produce],
	itm_apple, itm_orange, itm_lemon, itm_mushroom, itm_potato_raw,
	itm_blueberries, itm_strawberries, itm_tomato, itm_broccoli,
	itm_zucchini, NULL);

 setvector(
   mapitems[mi_cleaning],
	itm_salt_water, itm_bleach, itm_ammonia, itm_broom, itm_mop,
	itm_gloves_rubber, itm_mask_dust, itm_bottle_plastic, itm_sewing_kit, itm_thread,
	itm_rag, itm_scissors, itm_string_36, NULL);

 setvector(
   mapitems[mi_hardware],
	itm_superglue, itm_chain, itm_rope_6, itm_rope_30, itm_glass_sheet,
	itm_pipe, itm_nail, itm_hose, itm_string_36, itm_frame, itm_metal_tank,
	itm_wire, itm_wire_barbed, itm_duct_tape, itm_jerrycan, itm_lawnmower, itm_foot_crank,
	NULL);

 setvector(
   mapitems[mi_tools],
	itm_screwdriver, itm_hammer, itm_wrench, itm_saw, itm_hacksaw,
	itm_hammer_sledge, itm_xacto, itm_flashlight, itm_crowbar, itm_nailgun,
	itm_press, itm_puller, itm_boltcutters, NULL);

 setvector(
   mapitems[mi_bigtools],
	itm_broom, itm_mop, itm_hoe, itm_shovel, itm_chainsaw_off,
	itm_hammer_sledge, itm_jackhammer, itm_jacqueshammer, itm_welder, NULL);

 setvector(
   mapitems[mi_mischw],
	itm_2x4, itm_machete, itm_boots_steel, itm_hat_hard, itm_mask_filter,
	itm_glasses_safety, itm_bb, itm_bbgun, itm_beartrap, itm_two_way_radio,
	itm_radio, itm_hotplate, itm_extinguisher, itm_nailgun,
	itm_manual_mechanics, itm_manual_carpentry, itm_mag_carpentry,
	itm_duct_tape, itm_lawnmower, itm_boltcutters,
	itm_foot_crank, itm_cable, itm_textbook_mechanics, NULL);

 setvector(
   mapitems[mi_consumer_electronics],
	itm_amplifier, itm_antenna, itm_battery, itm_soldering_iron,
	itm_screwdriver, itm_processor, itm_RAM, itm_mp3, itm_flashlight,
	itm_radio, itm_hotplate, itm_receiver, itm_transponder, itm_tazer,
	itm_two_way_radio, itm_usb_drive, itm_manual_electronics, itm_cable, NULL);

 setvector(
   mapitems[mi_sports],
	itm_bandages, itm_aspirin, itm_bat, itm_bat_metal, itm_sneakers, itm_tshirt,
	itm_tank_top, itm_gloves_fingerless, itm_glasses_safety, itm_tshirt_fit,
	itm_armguard_soft, itm_armguard_hard,
	itm_goggles_swim, itm_goggles_ski, itm_hat_ball, itm_helmet_bike,
	itm_helmet_ball, itm_manual_brawl, itm_foot_crank, NULL);

 setvector(
   mapitems[mi_camping],
	itm_rope_30, itm_hatchet, itm_pot, itm_pan, itm_binoculars,
	itm_hotplate, itm_knife_combat, itm_machete, itm_vest, itm_backpack,
	itm_bb, itm_bolt_steel, itm_bbgun, itm_crossbow, itm_manual_knives,itm_manual_survival, 
	itm_manual_first_aid, itm_manual_traps, itm_lighter, itm_sewing_kit, itm_thread,
	itm_hammer, itm_flashlight, itm_water_purifier, itm_radio, itm_beartrap,
        itm_UPS_off, itm_string_36, itm_longbow, itm_compbow, itm_arrow_wood,
        itm_arrow_cf, itm_wire, itm_rollmat, itm_tent_kit, itm_canteen, NULL);


 setvector(
   mapitems[mi_allsporting],
	itm_aspirin, itm_bat, itm_bat_metal, itm_sneakers, itm_tshirt, itm_tank_top,
	itm_gloves_fingerless, itm_glasses_safety, itm_goggles_swim,
	itm_armguard_soft, itm_armguard_hard,
	itm_goggles_ski, itm_hat_ball, itm_helmet_bike, itm_helmet_ball,
	itm_manual_brawl, itm_rope_30, itm_hatchet, itm_pot, itm_pan,
	itm_binoculars, itm_hotplate, itm_knife_combat, itm_machete, itm_vest,
	itm_backpack, itm_bb, itm_bolt_steel, itm_bbgun, itm_crossbow,
	itm_manual_knives, itm_manual_first_aid, itm_manual_traps, itm_lighter,
	itm_sewing_kit, itm_thread, itm_hammer, itm_flashlight, itm_water_purifier,
	itm_radio, itm_beartrap, itm_extinguisher, itm_string_36, itm_longbow,
	itm_compbow, itm_arrow_wood, itm_arrow_cf, itm_rollmat, itm_tent_kit,
	itm_tshirt_fit, itm_foot_crank, NULL);

 setvector(
   mapitems[mi_alcohol],
	itm_whiskey, itm_vodka, itm_gin, itm_rum, itm_tequila, itm_triple_sec, NULL);

 setvector(
   mapitems[mi_pool_table],
	itm_pool_cue, itm_pool_ball, NULL);

 setvector(
   mapitems[mi_trash],
	itm_iodine, itm_meth, itm_heroin, itm_wrapper, itm_string_6, itm_chain,
	itm_glass_sheet, itm_stick, itm_muffler, itm_pipe, itm_bag_plastic,
	itm_bottle_plastic, itm_bottle_glass, itm_can_drink, itm_can_food,
	itm_box_small, itm_bubblewrap, itm_lighter, itm_syringe, itm_rag,
	itm_software_hacking, itm_carboy_plastic, NULL);

 setvector(
   mapitems[mi_ammo],
	itm_shot_bird, itm_shot_00, itm_shot_slug, itm_22_lr, itm_22_cb,
	itm_22_ratshot, itm_9mm, itm_9mmP, itm_9mmP2, itm_38_special,
	itm_38_super, itm_10mm, itm_40sw, itm_44magnum, itm_45_acp, itm_45_jhp,
	itm_45_super, itm_57mm, itm_46mm, itm_762_m43, itm_762_m87, itm_223,
	itm_556, itm_270, itm_3006, itm_308, itm_762_51, NULL);

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
	itm_american_180, itm_uzi, itm_tec9, itm_calico, itm_hk_mp5, itm_mac_10,
	itm_hk_ump45, itm_TDI, itm_fn_p90, itm_hk_mp7, NULL);

 setvector(
   mapitems[mi_assault],
	itm_hk_g3, itm_hk_g36, itm_ak47, itm_fn_fal, itm_acr, itm_ar15,
	itm_scar_l, itm_scar_h, itm_steyr_aug, itm_m249, NULL);

 setvector(
   mapitems[mi_allguns],
	itm_sig_mosquito, itm_sw_22, itm_glock_19, itm_usp_9mm, itm_sw_619,
	itm_taurus_38, itm_sig_40, itm_sw_610, itm_ruger_redhawk, itm_deagle_44,
	itm_usp_45, itm_m1911, itm_fn57, itm_hk_ucp, itm_shotgun_s,
	itm_shotgun_d, itm_remington_870, itm_mossberg_500, itm_saiga_12,
	itm_american_180, itm_uzi, itm_tec9, itm_calico, itm_hk_mp5, itm_mac_10,
	itm_hk_ump45, itm_TDI, itm_fn_p90, itm_hk_mp7, itm_marlin_9a,
	itm_ruger_1022, itm_browning_blr, itm_remington_700, itm_sks,
	itm_ruger_mini, itm_savage_111f, itm_hk_g3, itm_hk_g36, itm_ak47,
	itm_fn_fal, itm_acr, itm_ar15, itm_scar_l, itm_scar_h, itm_steyr_aug,
	itm_m249, NULL);

 setvector(
   mapitems[mi_gunxtras],
	itm_glasses_safety, itm_goggles_nv, itm_holster, itm_bootstrap,
	itm_mag_guns, itm_flashlight, itm_UPS_off, itm_silencer, itm_grip,
	itm_barrel_big, itm_barrel_small, itm_barrel_rifled, itm_clip, itm_spare_mag,
	itm_clip2, itm_stablizer, itm_blowback, itm_autofire, itm_retool_45,
	itm_retool_9mm, itm_retool_22, itm_retool_57, itm_retool_46,
	itm_retool_308, itm_retool_223, itm_tazer,itm_shot_hull,
        itm_9mm_casing, itm_38_casing, itm_40_casing, itm_44_casing, itm_45_casing,
        itm_57mm_casing, itm_46mm_casing, itm_762_casing, itm_223_casing,
        itm_3006_casing, itm_308_casing, itm_gunpowder,
        itm_shotgun_primer, itm_smpistol_primer, itm_lgpistol_primer,
        itm_smrifle_primer, itm_lgrifle_primer, itm_lead, itm_press, itm_puller, NULL);

 setvector(
   mapitems[mi_shoes],
	itm_sneakers, itm_boots, itm_flip_flops, itm_dress_shoes, itm_heels,
        itm_sneakers_fit, itm_boots_fit, itm_boots_winter_fit,
        itm_dress_shoes_fit, itm_heels_fit, NULL);

 setvector(
   mapitems[mi_pants],
	itm_jeans, itm_pants, itm_pants_leather, itm_pants_cargo, itm_skirt,
	itm_dress,
        itm_jeans_fit, itm_pants_fit, itm_pants_cargo_fit, NULL);

 setvector(
   mapitems[mi_shirts],
	itm_tshirt, itm_polo_shirt, itm_dress_shirt, itm_tank_top,
	itm_sweatshirt, itm_sweater, itm_hoodie, 
        itm_polo_shirt_fit, itm_sweatshirt_fit, itm_sweater_fit,
        itm_tshirt_fit, NULL);

 setvector(
   mapitems[mi_jackets],
	itm_jacket_light, itm_jacket_jean, itm_blazer, itm_jacket_leather,
	itm_coat_rain, itm_trenchcoat, itm_trenchcoat_fit, itm_jacket_light_fit,
        itm_jacket_leather_fit, NULL);

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
	itm_jeans, itm_pants, itm_suit, itm_tophat, itm_glasses_monocle,
	itm_pants_leather, itm_pants_cargo, itm_skirt, itm_tshirt,
	itm_polo_shirt, itm_dress_shirt, itm_tank_top, itm_sweatshirt,
	itm_sweater, itm_hoodie, itm_jacket_light, itm_jacket_jean,
	itm_blazer, itm_jacket_leather, itm_coat_winter, itm_peacoat,
	itm_gloves_light, itm_mittens, itm_gloves_wool, itm_gloves_winter,
	itm_gloves_leather, itm_scarf, itm_hat_cotton, itm_hat_knit,
	itm_hat_fur, itm_UPS_off, itm_jeans_fit, itm_pants_fit, 
        itm_pants_cargo_fit, itm_polo_shirt_fit, itm_sweatshirt_fit, 
        itm_sweater_fit,
        itm_jacket_light_fit, itm_tshirt_fit, itm_trenchcoat_fit,
        itm_trenchcoat_leather, itm_trenchcoat_leather_fit, NULL);

 setvector(
   mapitems[mi_novels],
	itm_novel_romance, itm_novel_spy, itm_novel_scifi, itm_novel_drama,
	NULL);

 setvector(
   mapitems[mi_manuals],
	itm_manual_brawl, itm_manual_knives, itm_manual_mechanics,
	itm_manual_speech, itm_manual_business, itm_manual_first_aid,
	itm_manual_computers, itm_cookbook, itm_manual_electronics,
	itm_manual_tailor, itm_manual_traps, itm_manual_carpentry,
        itm_manual_survival, NULL);

 setvector(
   mapitems[mi_textbooks],
	itm_textbook_computers, itm_textbook_electronics, itm_textbook_business,
	itm_textbook_chemistry, itm_textbook_carpentry, itm_SICP,
	itm_textbook_robots, itm_textbook_mechanics, NULL);

 setvector(
   mapitems[mi_cop_weapons],
	itm_baton, itm_kevlar, itm_vest, itm_gloves_leather, itm_mask_gas,
	itm_goggles_nv, itm_helmet_riot, itm_holster, itm_bootstrap,
	itm_armguard_hard,
	itm_shot_00, itm_9mm, itm_usp_9mm, itm_remington_870, itm_two_way_radio,
	itm_UPS_off, itm_tazer, NULL);

 setvector(
   mapitems[mi_cop_evidence],
	itm_weed, itm_coke, itm_meth, itm_heroin, itm_syringe, itm_electrohack,
	itm_knife_combat, itm_crowbar, itm_tazer, itm_software_hacking, NULL);

 setvector(
   mapitems[mi_hospital_lab],
	itm_blood, itm_iodine, itm_bleach, itm_bandages, itm_syringe,
	itm_canister_empty, itm_coat_lab, itm_gloves_medical, itm_mask_dust,
	itm_glasses_safety, itm_vacutainer, itm_usb_drive, NULL);

 setvector(
   mapitems[mi_hospital_samples],
	itm_blood, itm_vacutainer, NULL);

 setvector(
   mapitems[mi_surgery],
	itm_blood, itm_iodine, itm_bandages, itm_scalpel, itm_syringe,
	itm_gloves_medical, itm_mask_dust, itm_vacutainer, NULL);

 setvector(
   mapitems[mi_office],
	itm_cola, itm_aspirin, itm_cigar, itm_glasses_eye, itm_sunglasses, itm_glasses_reading,
	itm_purse, itm_mbag, itm_battery, itm_mag_news, itm_manual_business,
	itm_textbook_business, itm_lighter, itm_extinguisher, itm_flashlight,
	itm_radio, itm_bubblewrap, itm_coffee_raw, itm_usb_drive,
	itm_software_useless, NULL);

 setvector(
   mapitems[mi_vault],
	itm_purifier, itm_plut_cell, itm_ftk93, itm_nx17, itm_canister_goo,
	itm_UPS_off, itm_gold, itm_bionics_super, itm_plasma_engine,
	itm_minireactor, itm_alloy_plate, NULL);

 setvector(
   mapitems[mi_art],
	itm_fur, itm_katana, itm_petrified_eye, itm_spiral_stone, itm_rapier,
	itm_cane, itm_candlestick, itm_heels, itm_ring, itm_necklace, NULL);

 setvector(
   mapitems[mi_pawn],
	itm_cigar, itm_katana, itm_gold, itm_rapier, itm_cane, itm_suit,
	itm_mask_gas, itm_goggles_welding, itm_goggles_nv, itm_glasses_monocle,
	itm_tophat, itm_ruger_redhawk, itm_deagle_44, itm_m1911, itm_geiger_off,
	itm_UPS_off, itm_tazer, itm_mp3, itm_fur, itm_leather, itm_string_36,
	itm_chain, itm_steel_chunk, itm_spring, itm_steel_lump, itm_manhole_cover, itm_rock,
	itm_hammer_sledge, itm_ax, itm_knife_butcher, itm_knife_combat,
	itm_bat, itm_petrified_eye, itm_binoculars, itm_boots, itm_mocassins,
	itm_dress_shoes, itm_heels, itm_pants, itm_pants_army, itm_skirt,
	itm_jumpsuit, itm_dress, itm_dress_shirt, itm_sweater, itm_blazer,
	itm_jacket_leather, itm_coat_fur, itm_peacoat, itm_coat_lab,
	itm_helmet_army, itm_hat_fur, itm_holster, itm_bootstrap,
	itm_remington_870, itm_browning_blr, itm_remington_700, itm_sks,
	itm_novel_romance, itm_novel_spy, itm_novel_scifi, itm_novel_drama,
	itm_SICP, itm_textbook_robots, itm_extinguisher, itm_radio,
	itm_chainsaw_off, itm_jackhammer, itm_jacqueshammer, itm_ring, itm_necklace, itm_usb_drive,
	itm_broadsword, itm_morningstar, itm_helmet_plate, itm_cot, itm_rollmat, itm_tent_kit,
	itm_boots_fit, itm_boots_steel_fit, itm_boots_winter_fit,
        itm_dress_shoes_fit, itm_bat_metal, itm_jeans_fit, itm_pants_army_fit, 
        itm_polo_shirt_fit, itm_sweatshirt_fit, itm_sweater_fit,
        itm_jacket_light_fit, itm_tshirt_fit, itm_trenchcoat_fit, itm_lawnmower,
        itm_lawnmower_machete, itm_picklocks, NULL);

 setvector(
   mapitems[mi_mil_surplus], // NOT food or armor!
	itm_knife_combat, itm_binoculars, itm_bolt_steel, itm_crossbow,
	itm_mag_guns, itm_manual_brawl, itm_manual_knives, itm_cot,
	itm_manual_mechanics, itm_manual_first_aid, itm_manual_traps,
	itm_flashlight, itm_water_purifier, itm_two_way_radio, itm_radio,
	itm_geiger_off, itm_usb_drive, itm_canteen, itm_jerrycan, NULL);

 setvector(
   mapitems[mi_shelter],
	itm_water_clean, itm_soup, itm_chocolate, itm_ravioli, itm_can_beans,
	itm_can_spam, itm_can_tuna, itm_coffee_raw, itm_bandages, itm_1st_aid,
	itm_vitamins, itm_iodine, itm_dayquil, itm_screwdriver, itm_boots,
	itm_boots_winter, itm_jeans, itm_tshirt, itm_sweatshirt, itm_sweater,
	itm_coat_winter, itm_gloves_wool, itm_gloves_winter, itm_hat_knit,
	itm_backpack, itm_battery, itm_novel_scifi, itm_novel_drama,
	itm_manual_first_aid, itm_manual_tailor, itm_manual_carpentry,
	itm_lighter, itm_sewing_kit, itm_thread, itm_hammer, itm_extinguisher,
	itm_flashlight, itm_hotplate, itm_water_purifier, itm_radio, itm_rollmat, itm_tent_kit, 
  itm_canteen, NULL);

 setvector(
   mapitems[mi_mechanics],
        itm_wrench, itm_frame, itm_wheel, itm_motor, itm_combustion_small,
        itm_hacksaw, itm_welder, itm_motor, itm_motor_large, 
        itm_storage_battery, itm_solar_panel, itm_combustion_tiny, itm_jerrycan, NULL);

 setvector(
   mapitems[mi_chemistry],
	itm_iodine, itm_water_clean, itm_salt_water, itm_bleach, itm_ammonia,
	itm_mutagen, itm_purifier, itm_royal_jelly, itm_superglue,
	itm_bottle_glass, itm_syringe, itm_extinguisher, itm_hotplate,
	itm_software_medical, NULL);

 setvector(
   mapitems[mi_teleport],
	itm_screwdriver, itm_wrench, itm_jumpsuit, itm_mask_dust,
	itm_glasses_safety, itm_goggles_welding, itm_teleporter, itm_usb_drive,
	NULL);

 setvector(
   mapitems[mi_goo],
	itm_jumpsuit, itm_gloves_rubber, itm_mask_filter, itm_glasses_safety,
	itm_helmet_riot, itm_lighter, itm_canister_goo, NULL);

 setvector(
   mapitems[mi_cloning_vat],
	itm_fetus, itm_arm, itm_leg, NULL);

 setvector(
   mapitems[mi_dissection],
	itm_iodine, itm_bleach, itm_bandages, itm_string_6, itm_hacksaw,
	itm_xacto, itm_knife_butcher, itm_machete, itm_gloves_rubber,
	itm_bag_plastic, itm_syringe, itm_rag, itm_scissors, NULL);

 setvector(
   mapitems[mi_hydro],
	itm_blueberries, itm_strawberries, itm_tomato, itm_broccoli,
	itm_zucchini, itm_potato_raw, NULL);

 setvector(
   mapitems[mi_electronics],
	itm_superglue, itm_electrohack, itm_processor, itm_RAM,
	itm_power_supply, itm_amplifier, itm_transponder, itm_receiver,
	itm_antenna,
	itm_screwdriver, itm_mask_dust, itm_glasses_safety, itm_goggles_welding,
	itm_battery, itm_plut_cell, itm_manual_electronics,
	itm_textbook_electronics, itm_soldering_iron, itm_hotplate, itm_UPS_off,
	itm_usb_drive, itm_software_useless, NULL);

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
   mapitems[mi_bionics_common],
	itm_bionics_battery, itm_bionics_power, itm_bionics_tools, NULL);

 setvector(
   mapitems[mi_bots],
	itm_bot_manhack, itm_bot_turret, NULL);

 setvector(
   mapitems[mi_launchers],
	itm_40mm_concussive, itm_40mm_frag, itm_40mm_incendiary,
	itm_40mm_teargas, itm_40mm_smoke, itm_40mm_flashbang, itm_m79,
	itm_m320, itm_mgl, itm_m203, NULL);

 setvector(
   mapitems[mi_mil_rifles],
	itm_556, itm_556_incendiary, itm_762_51, itm_762_51_incendiary,
	itm_laser_pack, itm_12mm, itm_plasma, itm_m4a1, itm_scar_l, itm_scar_h,
	itm_m249, itm_ftk93, itm_nx17, itm_hk_g80, itm_plasma_rifle,
	itm_silencer, itm_clip, itm_spare_mag, itm_m203, itm_UPS_off, itm_u_shotgun,
	NULL);

 setvector(
   mapitems[mi_grenades],
	itm_grenade, itm_flashbang, itm_EMPbomb, itm_gasbomb, itm_smokebomb,
	itm_dynamite, itm_mininuke, itm_c4, NULL);

 setvector(
   mapitems[mi_mil_armor],
	itm_pants_army, itm_kevlar, itm_vest, itm_mask_gas, itm_goggles_nv,
	itm_helmet_army, itm_backpack, itm_UPS_off, itm_beltrig,
        itm_boots, itm_boots_fit, itm_pants_army_fit, itm_armguard_hard,
	itm_helmet_army, itm_backpack, itm_UPS_off, itm_beltrig, NULL);

 setvector(
   mapitems[mi_mil_food],
	itm_chocolate, itm_can_beans, itm_mre_beef, itm_mre_veggy, itm_1st_aid,
	itm_codeine, itm_antibiotics, itm_water_clean, itm_purifier, NULL);

 setvector(
   mapitems[mi_mil_food_nodrugs],
	itm_chocolate, itm_can_beans, itm_mre_beef, itm_mre_veggy, itm_1st_aid,
	itm_water_clean, NULL);

 setvector(
   mapitems[mi_bionics_mil],
	itm_bionics_battery, itm_bionics_power, itm_bionics_sensory,
	itm_bionics_combat_aug, itm_bionics_desert, itm_bionics_armor,
	itm_bionics_espionage, itm_bionics_defense, itm_bionics_medical,
	itm_bionics_super, itm_bionics_ranged, NULL);

 setvector(
   mapitems[mi_weapons],
	itm_chain, itm_hammer, itm_wrench, itm_hammer_sledge, itm_hatchet,
	itm_ax, itm_knife_combat, itm_pipe, itm_bat, itm_machete, itm_katana,
	itm_baton, itm_tazer, itm_rapier, itm_bat_metal, NULL);

 setvector(
   mapitems[mi_survival_armor],
	itm_boots_steel, itm_pants_cargo, itm_pants_army, itm_jumpsuit,
	itm_jacket_leather, itm_kevlar, itm_vest, itm_gloves_fingerless,
	itm_mask_filter, itm_mask_gas, itm_goggles_ski, itm_helmet_skid,
  itm_armguard_hard,
	itm_helmet_ball, itm_helmet_riot, itm_helmet_motor, itm_holster,
	itm_bootstrap, itm_UPS_off, itm_beltrig, 
        itm_pants_cargo_fit, itm_pants_army_fit,NULL);

 setvector(
   mapitems[mi_survival_tools],
	itm_bandages, itm_1st_aid, itm_caffeine, itm_iodine, itm_electrohack,
	itm_string_36, itm_rope_30, itm_chain, itm_binoculars,
	itm_bottle_plastic, itm_lighter, itm_sewing_kit, itm_thread, itm_extinguisher,
	itm_flashlight, itm_crowbar, itm_chainsaw_off, itm_beartrap,
	itm_grenade, itm_EMPbomb, itm_hotplate, itm_UPS_off, itm_canteen, NULL);

 setvector(
   mapitems[mi_sewage_plant],
	itm_1st_aid, itm_motor, itm_hose, itm_screwdriver, itm_wrench, itm_pipe,
	itm_boots, itm_jumpsuit, itm_coat_lab, itm_gloves_rubber,
	itm_mask_filter, itm_glasses_safety, itm_hat_hard, itm_extinguisher,
	itm_flashlight, itm_water_purifier, itm_two_way_radio,
	itm_bionics_tools, itm_bionics_hazmat, NULL);

 setvector(
   mapitems[mi_mine_storage],
	itm_rock, itm_coal, NULL);

 setvector(
   mapitems[mi_mine_equipment],
	itm_water_clean, itm_1st_aid, itm_rope_30, itm_chain, itm_boots_steel,
	itm_jumpsuit, itm_gloves_leather, itm_mask_filter, itm_mask_gas,
	itm_glasses_safety, itm_goggles_welding, itm_goggles_nv, itm_hat_hard,
	itm_backpack, itm_battery, itm_flashlight, itm_two_way_radio,
	itm_jackhammer, itm_jacqueshammer, itm_dynamite, itm_UPS_off, itm_bionics_tools,
	itm_bionics_construction, itm_jerrycan, NULL);


 setvector(
   mapitems[mi_spiral],
	itm_spiral_stone, itm_vortex_stone, NULL);

 setvector(
   mapitems[mi_radio],
	itm_cola, itm_caffeine, itm_cig, itm_weed, itm_amplifier,
	itm_transponder, itm_receiver, itm_antenna, itm_screwdriver,
	itm_battery, itm_mag_porn, itm_mag_tv, itm_manual_electronics,
	itm_lighter, itm_flashlight, itm_two_way_radio, itm_radio, itm_mp3,
	itm_usb_drive, NULL);

 setvector(
   mapitems[mi_toxic_dump_equipment],
	itm_1st_aid, itm_iodine, itm_canister_empty, itm_boots_steel,
	itm_hazmat_suit, itm_mask_gas, itm_hat_hard, itm_textbook_carpentry,
	itm_extinguisher, itm_radio, itm_geiger_off, itm_UPS_off,
	itm_bionics_hazmat, NULL);

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
	itm_rock, itm_jackhammer, itm_jacqueshammer, itm_flashlight, itm_dynamite, NULL);

 setvector(
   mapitems[mi_spider],
	itm_corpse, itm_mutagen, itm_purifier, itm_meat, itm_meat_tainted,
	itm_arm, itm_leg, itm_1st_aid, itm_codeine, itm_oxycodone, itm_weed,
	itm_coke, itm_wrapper, itm_fur, itm_leather, itm_id_science,
	itm_id_military, itm_rope_30, itm_stick, itm_hatchet, itm_ax,
	itm_bee_sting, itm_chitin_piece, itm_vest, itm_mask_gas, itm_goggles_nv,
	itm_hat_boonie, itm_helmet_riot, itm_bolt_steel, itm_shot_00,
	itm_762_m87, itm_556, itm_556_incendiary, itm_3006_incendiary,
	itm_762_51, itm_762_51_incendiary, itm_saiga_12, itm_hk_mp5, itm_TDI,
	itm_savage_111f, itm_sks, itm_ak47, itm_m4a1, itm_steyr_aug, itm_v29,
	itm_nx17, itm_flamethrower, itm_flashlight, itm_radio, itm_geiger_off,
	itm_teleporter, itm_canister_goo, itm_dynamite, itm_mininuke,
	itm_bot_manhack, itm_UPS_off, itm_bionics_battery, itm_bionics_tools,
	itm_arrow_cf, NULL);

 setvector(
   mapitems[mi_ant_food],
	itm_meat, itm_veggy, itm_meat_tainted, itm_veggy_tainted, itm_apple,
	itm_orange, itm_mushroom, itm_blueberries, itm_strawberries,
	itm_tomato, itm_broccoli, itm_zucchini, itm_potato_raw, itm_honeycomb,
	itm_royal_jelly, itm_arm, itm_leg, itm_rock, itm_stick, NULL);

 setvector(
   mapitems[mi_ant_egg],
	itm_ant_egg, NULL);	//TODO: More items here?

 setvector(
   mapitems[mi_biollante],
	itm_biollante_bud, NULL);

 setvector(
   mapitems[mi_bugs],
	itm_chitin_piece, NULL);

 setvector(
   mapitems[mi_bees],
	itm_bee_sting, itm_chitin_piece, NULL);
 
 setvector(
   mapitems[mi_wasps],
	itm_wasp_sting, itm_chitin_piece, NULL);

 setvector(
   mapitems[mi_robots],
	itm_processor, itm_RAM, itm_power_supply, itm_amplifier,
	itm_transponder, itm_receiver, itm_antenna, itm_steel_chunk, itm_spring,
	itm_steel_lump, itm_motor, itm_battery, itm_plut_cell, NULL);

 setvector(
   mapitems[mi_helicopter],
	itm_chain, itm_power_supply, itm_antenna, itm_steel_chunk, itm_spring,
	itm_steel_lump, itm_frame, itm_steel_plate, itm_spiked_plate,
	itm_hard_plate, itm_motor, itm_motor_large, itm_hose, itm_pants_army,
	itm_jumpsuit, itm_kevlar, itm_mask_gas, itm_helmet_army, itm_battery,
	itm_plut_cell, itm_m249, itm_combustion_large, itm_extinguisher,
	itm_two_way_radio, itm_radio, itm_UPS_off, itm_beltrig, 
        itm_pants_army_fit, itm_boots_fit, NULL);

// TODO: Replace kevlar with the ceramic plate armor
 setvector(
   mapitems[mi_military],
	itm_water_clean, itm_mre_beef, itm_mre_veggy, itm_bandages, itm_1st_aid,
	itm_iodine, itm_codeine, itm_cig, itm_knife_combat, itm_boots_steel,
	itm_pants_army, itm_kevlar, itm_vest, itm_gloves_fingerless,
	itm_mask_gas, itm_glasses_safety, itm_goggles_nv, itm_hat_boonie,
	itm_armguard_hard,
	itm_helmet_army, itm_backpack, itm_holster, itm_bootstrap, itm_9mm,
	itm_45_acp, itm_556, itm_556_incendiary, itm_762_51,
	itm_762_51_incendiary, itm_laser_pack, itm_40mm_concussive,
	itm_40mm_frag, itm_40mm_incendiary, itm_40mm_teargas, itm_40mm_smoke,
	itm_40mm_flashbang, itm_usp_9mm, itm_usp_45, itm_m4a1, itm_scar_l,
	itm_scar_h, itm_m249, itm_ftk93, itm_nx17, itm_m320, itm_mgl,
	itm_silencer, itm_clip, itm_lighter, itm_flashlight, itm_two_way_radio,
	itm_landmine, itm_grenade, itm_flashbang, itm_EMPbomb, itm_gasbomb,
	itm_smokebomb, itm_UPS_off, itm_tazer, itm_c4, itm_hk_g80, itm_12mm,
	itm_binoculars, itm_u_shotgun, itm_beltrig, itm_pants_army_fit, 
        itm_spare_mag, itm_canteen, itm_jerrycan, NULL);


 setvector(
   mapitems[mi_science],
	itm_water_clean, itm_bleach, itm_ammonia, itm_mutagen, itm_purifier,
	itm_iodine, itm_inhaler, itm_adderall, itm_id_science, itm_electrohack,
	itm_RAM, itm_screwdriver, itm_canister_empty, itm_coat_lab,
	itm_gloves_medical, itm_mask_dust, itm_mask_filter, itm_glasses_eye, itm_sunglasses,
	itm_glasses_safety, itm_textbook_computers, itm_textbook_electronics,
	itm_textbook_chemistry, itm_SICP, itm_textbook_robots,
	itm_soldering_iron, itm_geiger_off, itm_teleporter, itm_canister_goo,
	itm_EMPbomb, itm_pheromone, itm_portal, itm_bot_manhack, itm_UPS_off,
	itm_tazer, itm_bionics_hazmat, itm_plasma, itm_usb_drive,
	itm_software_useless, itm_canteen, NULL);

 setvector(
   mapitems[mi_rare],
	itm_mutagen, itm_purifier, itm_royal_jelly, itm_fetus, itm_id_science,
	itm_id_military, itm_electrohack, itm_processor, itm_armor_chitin,
	itm_plut_cell, itm_laser_pack, itm_m249, itm_v29, itm_ftk93, itm_nx17,
	itm_conversion_battle, itm_conversion_sniper, itm_canister_goo,
	itm_mininuke, itm_portal, itm_c4, itm_12mm, itm_hk_g80,
	itm_plasma, itm_plasma_rifle, NULL);

 setvector(
   mapitems[mi_stash_food],
	itm_water_clean, itm_cola, itm_jerky, itm_ravioli, itm_can_beans,
	itm_can_corn, itm_can_spam, NULL);

 setvector(
   mapitems[mi_stash_ammo],
	itm_bolt_steel, itm_shot_00, itm_shot_slug, itm_22_lr, itm_9mm,
	itm_38_super, itm_10mm, itm_44magnum, itm_45_acp, itm_57mm, itm_46mm,
	itm_762_m87, itm_556, itm_3006, itm_762_51, itm_arrow_cf, NULL);

 setvector(
   mapitems[mi_stash_wood],
	itm_stick, itm_ax, itm_saw, itm_2x4, NULL);

 setvector(
   mapitems[mi_stash_drugs],
	itm_pills_sleep, itm_oxycodone, itm_xanax, itm_adderall, itm_weed,
	itm_coke, itm_meth, itm_heroin, itm_crack, itm_crackpipe, NULL);

 setvector(
   mapitems[mi_drugdealer],
	itm_energy_drink, itm_whiskey, itm_jerky, itm_bandages, itm_caffeine,
	itm_oxycodone, itm_adderall, itm_cig, itm_weed, itm_coke, itm_meth,
	itm_heroin, itm_syringe, itm_electrohack, itm_hatchet, itm_nailboard,
	itm_knife_combat, itm_bat, itm_machete, itm_katana, itm_pants_cargo,
	itm_hoodie, itm_gloves_fingerless, itm_backpack, itm_holster,
	itm_armguard_soft, itm_armguard_hard,
	itm_shot_00, itm_9mm, itm_45_acp, itm_glock_19, itm_shotgun_sawn,
	itm_uzi, itm_tec9, itm_mac_10, itm_silencer, itm_clip2, itm_autofire,
	itm_mag_porn, itm_lighter, itm_crowbar, itm_pipebomb, itm_grenade,
	itm_mininuke, itm_crack, itm_crackpipe, itm_pants_cargo_fit, itm_spare_mag, NULL);

 setvector(
   mapitems[mi_wreckage],
	itm_chain, itm_steel_chunk, itm_spring, itm_steel_lump, itm_frame, itm_rock, NULL);

 setvector(
   mapitems[mi_npc_hacker],
	itm_energy_drink, itm_adderall, itm_electrohack, itm_usb_drive,
	itm_battery, itm_manual_computers, itm_textbook_computers,
	itm_SICP, itm_soldering_iron, NULL);

// This one kind of an inverted list; what an NPC will NOT carry
 setvector(
   mapitems[mi_trader_avoid],
	itm_null, itm_corpse, itm_fire, itm_toolset, itm_meat, itm_veggy,
	itm_meat_tainted, itm_veggy_tainted, itm_meat_cooked, itm_veggy_cooked,
	itm_mushroom_poison, itm_spaghetti_cooked, itm_macaroni_cooked,
	itm_fetus, itm_arm, itm_leg, itm_wrapper, itm_manhole_cover, itm_rock,
	itm_stick, itm_bag_plastic, itm_flashlight_on, itm_radio_on,
	itm_chainsaw_on, itm_pipebomb_act, itm_grenade_act, itm_flashbang_act,
	itm_EMPbomb_act, itm_gasbomb_act, itm_smokebomb_act, itm_molotov_lit,
	itm_dynamite_act, itm_mininuke_act, itm_UPS_on, itm_mp3_on, itm_c4armed,
        itm_apparatus, itm_grack, itm_brazier,
	NULL);
}
