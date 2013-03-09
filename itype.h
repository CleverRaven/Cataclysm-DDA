#ifndef _ITYPE_H_
#define _ITYPE_H_

#include <string>
#include <vector>
#include <sstream>
#include "color.h"
#include "enums.h"
#include "iuse.h"
#include "pldata.h"
#include "bodypart.h"
#include "skill.h"
#include "bionics.h"
#include "artifact.h"

// mfb(n) converts a flag to its appropriate position in covers's bitfield
#ifndef mfb
#define mfb(n) long(1 << (n))
#endif

// for use in category specific inventory lists
enum item_cat
{
  IC_NULL = 0,
  IC_COMESTIBLE,
  IC_AMMO,
  IC_ARMOR,
  IC_GUN,
  IC_BOOK,
  IC_TOOL,
  IC_CONTAINER
};


enum itype_id {
itm_null = 0,
itm_corpse,
// Special pseudoitems
itm_fire, itm_toolset, itm_apparatus,
// Drinks
itm_water, itm_water_clean, itm_sewage, itm_salt_water, itm_oj, itm_apple_cider,
 itm_energy_drink, itm_cola, itm_rootbeer, itm_milk, itm_V8, itm_broth,
 itm_soup, itm_whiskey, itm_vodka, itm_gin, itm_rum, itm_tequila, itm_triple_sec,
 itm_long_island, itm_beer, itm_bleach,
 itm_ammonia, itm_mutagen, itm_purifier, itm_tea, itm_coffee, itm_blood,
// Monster Meats
itm_bone, itm_plant_sac, itm_meat, itm_veggy, itm_human_flesh, itm_veggy_wild, itm_meat_tainted, itm_veggy_tainted,
itm_meat_cooked, itm_veggy_cooked, itm_veggy_wild_cooked,
// Food
itm_apple, itm_orange, itm_lemon, itm_chips, itm_chips2, itm_chips3,
 itm_pretzels, itm_chocolate,
 itm_jerky, itm_sandwich_t, itm_candy, itm_mushroom, itm_mushroom_poison,
 itm_mushroom_magic, itm_blueberries, itm_strawberries, itm_tomato,
 itm_broccoli, itm_zucchini, itm_corn, itm_frozen_dinner, itm_cooked_dinner,
 itm_spaghetti_raw, itm_spaghetti_cooked, itm_macaroni_raw, itm_macaroni_cooked,
 itm_ravioli, itm_sauce_red, itm_sauce_pesto, itm_can_beans, itm_can_corn,
 itm_can_spam, itm_can_pineapple, itm_can_coconut, itm_can_sardine,
 itm_can_tuna, itm_can_catfood, itm_honeycomb, itm_wax, itm_royal_jelly, itm_fetus,
 itm_arm, itm_leg, itm_ant_egg, itm_marloss_berry, itm_flour, itm_sugar,
 itm_salt, itm_potato_raw, itm_potato_baked, itm_bread, itm_pie, itm_pizza,
 itm_mre_beef, itm_mre_veggy, itm_tea_raw, itm_coffee_raw,
// Medication
itm_bandages, itm_1st_aid, itm_vitamins, itm_aspirin, itm_caffeine,
 itm_pills_sleep, itm_iodine, itm_dayquil, itm_nyquil, itm_inhaler, itm_codeine,
 itm_oxycodone, itm_tramadol, itm_xanax, itm_adderall, itm_thorazine,
 itm_prozac, itm_cig, itm_weed, itm_coke, itm_meth, itm_heroin, itm_cigar,
 itm_antibiotics, itm_poppy_sleep, itm_poppy_pain, itm_crack, /*itm_grack,*/
// Do-nothing / Melee weapons
itm_wrapper, itm_withered, itm_syringe, itm_fur, itm_leather, itm_superglue,
 itm_id_science, itm_id_military, itm_electrohack, itm_string_6, itm_string_36,
 itm_rope_6, itm_rope_30, itm_chain, itm_processor, itm_RAM, itm_power_supply,
 itm_amplifier, itm_transponder, itm_receiver, itm_burnt_out_bionic,
 itm_antenna, itm_steel_chunk,
 itm_steel_lump, itm_scrap, itm_hose, itm_glass_sheet, itm_manhole_cover, itm_rock,
 itm_stick, itm_broom,
 itm_hammer_sledge, itm_hatchet, itm_nailboard, itm_nailbat,
 itm_pot, itm_pan, itm_knife_butter, itm_2x4, itm_muffler,
 itm_pipe, itm_bat, itm_bat_metal,
 itm_spear_wood, itm_baton,
 itm_bee_sting, itm_wasp_sting, itm_chitin_piece, itm_biollante_bud,
 itm_canister_empty, itm_gold, itm_coal, itm_petrified_eye, itm_spiral_stone,
 itm_cane, itm_binoculars, itm_usb_drive,
 itm_mace, itm_morningstar, itm_pool_cue, itm_pool_ball, itm_candlestick,
 itm_carspike, itm_carblade, itm_wire, itm_wire_barbed, itm_rebar, itm_log,
 itm_splinter, itm_skewer, itm_crackpipe, itm_torch_done,
 itm_spring, itm_lawnmower, itm_sheet, itm_broketent, itm_element,
 itm_television, itm_pilot_light, itm_toaster, itm_microwave, itm_laptop,
 itm_fan, itm_ceramic_plate, itm_ceramic_bowl, itm_ceramic_cup,
 itm_glass_plate, itm_glass_bowl, itm_glass, itm_tin_plate, itm_fork, itm_spork,
 itm_foon, itm_rag_bloody, itm_clock, itm_clockworks,
 itm_javelin,
// Vehicle parts
 itm_frame,
 itm_wheel, itm_wheel_wide, itm_wheel_bicycle, itm_wheel_motorbike, itm_wheel_small,
 itm_seat, itm_vehicle_controls,
 itm_1cyl_combustion, itm_v2_combustion, itm_i4_combustion,
 itm_v6_combustion, itm_v8_combustion,
 itm_motor, itm_motor_large, itm_plasma_engine, itm_foot_crank,
 itm_metal_tank, itm_storage_battery, itm_minireactor, itm_solar_panel,
 itm_steel_plate, itm_alloy_plate, itm_spiked_plate, itm_hard_plate,
// Footwear
 itm_socks, itm_socks_wool,
 itm_sneakers, itm_boots, itm_boots_steel, itm_boots_winter, itm_mocassins,
 itm_flip_flops, itm_dress_shoes, itm_heels,
 itm_sneakers_fit, itm_boots_fit, itm_boots_steel_fit, itm_boots_winter_fit,
 itm_dress_shoes_fit, itm_heels_fit, itm_boots_chitin,
// Legwear
 itm_shorts, itm_shorts_cargo,
 itm_jeans, itm_pants, itm_pants_leather, itm_pants_cargo, itm_pants_army, itm_pants_ski,
 itm_long_underpants, itm_skirt,
 itm_jeans_fit, itm_pants_fit, itm_pants_cargo_fit, itm_pants_army_fit,
// Full-body clothing
itm_jumpsuit, itm_dress, itm_armor_chitin, itm_suit, itm_hazmat_suit,
 itm_armor_plate,
// Torso clothing
itm_tshirt, itm_polo_shirt, itm_dress_shirt, itm_tank_top, itm_sweatshirt,
 itm_sweater, itm_hoodie, itm_under_armor, itm_jacket_light, itm_jacket_jean, itm_blazer,
 itm_jacket_leather, itm_kevlar, itm_coat_rain, itm_poncho, itm_trenchcoat, itm_trenchcoat_leather,
 itm_coat_winter, itm_coat_fur, itm_peacoat, itm_vest, itm_beltrig, itm_coat_lab,
 itm_tshirt_fit, itm_polo_shirt_fit, itm_hoodie_fit, itm_sweatshirt_fit, itm_sweater_fit,
 itm_jacket_light_fit, itm_jacket_leather_fit, itm_trenchcoat_fit, itm_trenchcoat_leather_fit,
// Arm clothing
itm_armguard_soft, itm_armguard_hard, itm_armguard_chitin, itm_armguard_metal,
// Gloves
itm_gloves_liner, itm_gloves_light, itm_mittens, itm_gloves_wool, itm_gloves_winter,
 itm_gloves_leather, itm_gloves_fingerless, itm_gloves_rubber,
 itm_gloves_medical, itm_fire_gauntlets, itm_gauntlets_chitin,
// Masks
itm_mask_dust, itm_bandana, itm_scarf, itm_mask_filter, itm_mask_gas,
// Eyewear
itm_glasses_eye, itm_glasses_reading, itm_glasses_safety, itm_goggles_swim,
 itm_goggles_ski, itm_goggles_welding, itm_goggles_nv, itm_glasses_monocle, itm_sunglasses,
// Headwear
itm_hat_ball, itm_hat_boonie, itm_hat_cotton, itm_hat_knit, itm_hat_hunting, itm_hat_fur, itm_balclava, itm_hat_hard,
 itm_helmet_bike, itm_helmet_skid, itm_helmet_ball, itm_helmet_army, itm_helmet_riot,
 itm_helmet_motor, itm_helmet_chitin, itm_helmet_plate, itm_tophat,
// High-storage
itm_backpack, itm_rucksack, itm_purse, itm_mbag, itm_fanny, itm_holster, itm_bootstrap,
itm_ragpouch, itm_leather_pouch,
// Decorative
itm_ring, itm_necklace,
// Ammunition
 itm_battery, itm_thread, itm_sinew, itm_plant_fibre, itm_duct_tape, itm_cable, itm_plut_cell,
 itm_nail, itm_bb, itm_arrow_wood, itm_arrow_cf,
 itm_bolt_wood, itm_bolt_steel, itm_shot_bird, itm_shot_00, itm_shot_slug,
 itm_shot_he, itm_22_lr, itm_22_cb, itm_22_ratshot, itm_9mm, itm_9mmP,
 itm_9mmP2, itm_38_special, itm_38_super, itm_10mm, itm_40sw, itm_44magnum,
 itm_45_acp, itm_45_jhp, itm_45_super, itm_57mm, itm_46mm, itm_762_m43,
 itm_762_m87, itm_223, itm_556, itm_556_incendiary, itm_270, itm_3006,
 itm_3006_incendiary, itm_308, itm_762_51, itm_762_51_incendiary,
 itm_laser_pack, itm_40mm_concussive, itm_40mm_frag, itm_40mm_incendiary,
 itm_40mm_teargas, itm_40mm_smoke, itm_40mm_flashbang, itm_12mm, itm_plasma,
 itm_charge_shot,
 itm_shot_hull,
 itm_9mm_casing, itm_38_casing, itm_40_casing, itm_44_casing, itm_45_casing,
 itm_57mm_casing, itm_46mm_casing, itm_762_casing, itm_223_casing,
 itm_3006_casing, itm_308_casing, itm_40mm_casing, itm_gunpowder,
 itm_shotgun_primer, itm_smpistol_primer, itm_lgpistol_primer,
 itm_smrifle_primer, itm_lgrifle_primer, itm_lead, itm_incendiary, itm_gasoline,
// Guns
itm_nailgun, itm_bbgun, itm_crossbow, itm_compbow, itm_longbow, itm_rifle_22,
 itm_rifle_9mm, itm_smg_9mm, itm_smg_45, itm_sig_mosquito, itm_sw_22,
 itm_glock_19, itm_usp_9mm, itm_sw_619, itm_taurus_38, itm_sig_40, itm_sw_610,
 itm_ruger_redhawk, itm_deagle_44, itm_usp_45, itm_m1911, itm_fn57, itm_hk_ucp,
 itm_shotgun_sawn, itm_saiga_sawn, itm_shotgun_s, itm_shotgun_d,  itm_remington_870,
 itm_mossberg_500, itm_saiga_12, itm_american_180, itm_uzi, itm_tec9,
 itm_calico, itm_hk_mp5, itm_mac_10, itm_hk_ump45, itm_TDI, itm_fn_p90,
 itm_hk_mp7, itm_marlin_9a, itm_ruger_1022, itm_browning_blr,
 itm_remington_700, itm_sks, itm_ruger_mini, itm_savage_111f, itm_hk_g3,
 itm_hk_g36, itm_ak47, itm_fn_fal, itm_acr, itm_ar15, itm_m4a1, itm_scar_l,
 itm_scar_h, itm_steyr_aug, itm_m249, itm_v29, itm_ftk93, itm_nx17,
 itm_flamethrower_simple, itm_flamethrower, itm_launcher_simple, itm_m79,
 itm_m320, itm_mgl, itm_coilgun, itm_hk_g80, itm_plasma_rifle,
 itm_revolver_shotgun,
// Gun modifications
itm_silencer, itm_grip, itm_barrel_big, itm_barrel_small, itm_barrel_rifled,
 itm_clip, itm_clip2, itm_spare_mag, itm_stablizer, itm_blowback, itm_autofire,
 itm_retool_45, itm_retool_9mm, itm_retool_22, itm_retool_57, itm_retool_46,
 itm_retool_308, itm_retool_223, itm_conversion_battle, itm_conversion_sniper,
 itm_m203, itm_bayonet, itm_u_shotgun, itm_gun_crossbow,
// Books
 itm_mag_porn, itm_mag_tv, itm_mag_news, itm_mag_cars, itm_mag_cooking,
 itm_mag_carpentry,
 itm_mag_guns, itm_mag_archery, itm_novel_romance, itm_novel_spy, itm_novel_scifi,
 itm_novel_drama, itm_manual_brawl, itm_manual_knives, itm_manual_mechanics, itm_manual_survival,
 itm_manual_speech, itm_manual_business, itm_manual_first_aid,
 itm_manual_computers, itm_cookbook, itm_manual_electronics,
 itm_manual_tailor, itm_manual_traps, itm_manual_carpentry,
 itm_textbook_computers, itm_textbook_electronics, itm_textbook_business, itm_textbook_mechanics,
 itm_textbook_chemistry, itm_textbook_carpentry, itm_SICP, itm_textbook_robots,
// Containers
itm_bag_plastic, itm_bottle_plastic, itm_bottle_glass,
 itm_can_drink, itm_can_food, itm_box_small,
 itm_canteen, itm_jerrycan, itm_carboy_plastic, itm_flask_glass, itm_waterskin,
// Tools
itm_lighter, itm_sewing_kit, itm_scissors, itm_hammer, itm_extinguisher,
 itm_flashlight, itm_flashlight_on, itm_hotplate, itm_soldering_iron,
 itm_water_purifier, itm_two_way_radio, itm_radio, itm_radio_on, itm_crowbar,
 itm_hoe, itm_shovel, itm_chainsaw_off, itm_chainsaw_on, itm_jackhammer, itm_jacqueshammer,
 itm_bubblewrap, itm_beartrap, itm_board_trap, itm_tripwire, itm_crossbow_trap,
 itm_shotgun_trap, itm_blade_trap, itm_landmine, itm_geiger_off, itm_geiger_on,
 itm_teleporter, itm_canister_goo, itm_pipebomb, itm_pipebomb_act, itm_grenade,
 itm_grenade_act, itm_flashbang, itm_flashbang_act, itm_EMPbomb,
 itm_EMPbomb_act, itm_scrambler, itm_scrambler_act, itm_gasbomb, itm_gasbomb_act, itm_smokebomb,
 itm_smokebomb_act, itm_molotov, itm_molotov_lit, itm_acidbomb,
 itm_acidbomb_act, itm_dynamite, itm_dynamite_act, itm_mininuke,
 itm_mininuke_act, itm_pheromone, itm_portal, itm_bot_manhack, itm_bot_turret,
 itm_UPS_off, itm_UPS_on, itm_tazer, itm_mp3, itm_mp3_on, itm_vortex_stone,
 itm_dogfood, itm_boobytrap, itm_c4, itm_c4armed, itm_dog_whistle,
 itm_vacutainer, itm_welder, itm_cot, itm_rollmat, itm_xacto, itm_scalpel,
 itm_machete, itm_katana, itm_spear_knife, itm_rapier, itm_pike, itm_broadsword,
 itm_lawnmower_blade, itm_lawnmower_machete, itm_lawnmower_halberd, itm_knife_steak,
 itm_knife_butcher, itm_knife_combat, itm_saw, itm_ax, itm_hacksaw,
 itm_tent_kit, itm_torch, itm_torch_lit, itm_candle, itm_candle_lit,
 itm_brazier, itm_puller, itm_press, itm_screwdriver, itm_wrench,
 itm_boltcutters, itm_mop, itm_picklocks, itm_pickaxe, itm_spray_can, itm_rag,
 itm_pda, itm_pda_flashlight, itm_pockknife, itm_needle_bone,
 itm_primitive_hammer, itm_primitive_axe, itm_primitive_shovel,
// Bionics containers
itm_bionics_battery,
 //power sources
 itm_bionics_solar,   itm_bionics_batteries,
 itm_bionics_metabolics,   itm_bionics_furnace, itm_bionics_ethanol,
 // utility
 itm_bionics_toolset,      itm_bionics_storage, itm_bionics_flashlight,
 itm_bionics_lighter,      itm_bionics_magnet,
 // neuro
 itm_bionics_memory,       itm_bionics_painkiller,itm_bionics_alarm,
 // sensory
 itm_bionics_ears,          itm_bionics_eye_enhancer, itm_bionics_night_vision,
 itm_bionics_infrared,      itm_bionics_scent_vision,
 // cbm: aquatic
 itm_bionics_membrane, itm_bionics_gills,
 // cbm: combat augs
 itm_bionics_targeting, itm_bionics_ground_sonar,
 // cbm: hazmat
 itm_bionics_purifier, itm_bionics_climate, itm_bionics_heatsink, itm_bionics_blood_filter,
 // nutritional
 itm_bionics_recycler, itm_bionics_digestion, itm_bionics_evap, itm_bionics_water_extractor,
 // desert survival (all dupes)
 // melee:
 itm_bionics_shock, itm_bionics_heat_absorb, itm_bionics_claws,
 // armor:
 itm_bionics_carbon, itm_bionics_armor_head, itm_bionics_armor_torso,
 itm_bionics_armor_arms, itm_bionics_armor_legs,
 // espionage
 itm_bionics_face_mask, itm_bionics_scent_mask, itm_bionics_cloak, itm_bionics_fingerhack,
 //defense
 itm_bionics_ads, itm_bionics_ods,
 // medical
 itm_bionics_nanobots, itm_bionics_blood_anal,
 // construction
 itm_bionics_resonator, itm_bionics_hydraulics,
 // super soldier
 itm_bionics_time_freeze, itm_bionics_teleport,
 // ranged combat
 itm_bionics_blaster, itm_bionics_laser, itm_bionics_emp,

// Software
itm_software_useless, itm_software_hacking, itm_software_medical,
 itm_software_math, itm_software_blood_data,
// MacGuffins!
itm_note,
//-----Jovan's. Flowers!-----
itm_poppy_flower, itm_poppy_bud,
//---------------------------

// Static (non-random) artifacts should go here.
num_items,
// These shouldn't be counted among "normal" items; thus, they are outside the
// bounds of num_items
itm_bio_claws, itm_bio_fusion, itm_bio_blaster,
// Unarmed Combat Styles
itm_style_karate, itm_style_aikido, itm_style_judo, itm_style_tai_chi,
 itm_style_capoeira, itm_style_krav_maga, itm_style_muay_thai,
 itm_style_ninjutsu, itm_style_taekwando, itm_style_tiger, itm_style_crane,
 itm_style_leopard, itm_style_snake, itm_style_dragon, itm_style_centipede,
 itm_style_venom_snake, itm_style_scorpion, itm_style_lizard, itm_style_toad,
 itm_style_zui_quan,
num_all_items
};

// IMPORTANT: If adding a new AT_*** ammotype, add it to the ammo_name function
//  at the end of itypedef.cpp
enum ammotype {
AT_NULL, AT_THREAD,
AT_BATT, AT_PLUT,
AT_NAIL, AT_BB, AT_BOLT, AT_ARROW,
AT_SHOT,
AT_22, AT_9MM, AT_38, AT_40, AT_44, AT_45,
AT_57, AT_46,
AT_762, AT_223, AT_3006, AT_308,
AT_40MM,
AT_GAS,
AT_FUSION,
AT_MUSCLE,
AT_12MM,
AT_PLASMA,
NUM_AMMO_TYPES
};

enum software_type {
SW_NULL,
SW_USELESS,
SW_HACKING,
SW_MEDICAL,
SW_SCIENCE,
SW_DATA,
NUM_SOFTWARE_TYPES
};

enum item_flag {
IF_NULL,

IF_LIGHT_4,	// Provides 4 tiles of light
IF_LIGHT_8,	// Provides 8 tiles of light

IF_FIRE,        // Chance to set fire to tiles/enemies
IF_SPEAR,	// Cutting damage is actually a piercing attack
IF_STAB,	// This weapon *can* pierce, but also has normal cutting
IF_WRAP,	// Can wrap around your target, costing you and them movement
IF_MESSY,	// Splatters blood, etc.
IF_RELOAD_ONE,	// Reload cartridge by cartridge (e.g. most shotguns)
IF_STR_RELOAD,  // Reloading time is reduced by Strength * 20
IF_STR8_DRAW,   // Requires strength 8 to draw
IF_STR10_DRAW,  // Requires strength 10 to draw
IF_USE_UPS,	// Draws power from a UPS
IF_RELOAD_AND_SHOOT, // Reloading and shooting is one action
IF_FIRE_100,	// Fires 100 rounds at once! (e.g. flamethrower)
IF_GRENADE,	// NPCs treat this as a grenade
IF_CHARGE,	// For guns; charges up slowly
IF_SHOCK,   // Stuns and damages enemies, powers up shockers.

IF_UNARMED_WEAPON, // Counts as an unarmed weapon
IF_NO_UNWIELD, // Impossible to unwield, e.g. bionic claws

// Weapon mode flags
IF_MODE_AUX, // A gunmod with a firing mode
IF_MODE_BURST, // A burst of attacks

NUM_ITEM_FLAGS
};

enum ammo_effect {
AMMO_FLAME,		// Sets fire to terrain and monsters
AMMO_INCENDIARY,	// Sparks explosive terrain
AMMO_EXPLOSIVE,		// Small explosion
AMMO_FRAG,		// Frag explosion
AMMO_NAPALM,		// Firey explosion
AMMO_EXPLOSIVE_BIG,	// Big explosion!
AMMO_TEARGAS,		// Teargas burst
AMMO_SMOKE,  		// Smoke burst
AMMO_TRAIL,		// Leaves a trail of smoke
AMMO_FLASHBANG,		// Disorients and blinds
AMMO_STREAM,		// Doesn't stop once it hits a monster
NUM_AMMO_EFFECTS
};


enum technique_id {
TEC_NULL,
// Offensive Techniques
TEC_SWEEP,	// Crits may make your enemy fall & miss a turn
TEC_PRECISE,	// Crits are painful and stun
TEC_BRUTAL,	// Crits knock the target back
TEC_GRAB,	// Hit may allow a second unarmed attack attempt
TEC_WIDE,	// Attacks adjacent oppoents
TEC_RAPID,	// Hits faster
TEC_FEINT,	// Misses take less time
TEC_THROW,	// Attacks may throw your opponent
TEC_DISARM,	// Remove an NPC's weapon
// Defensive Techniques
TEC_BLOCK,	// Block attacks, reducing them to 25% damage
TEC_BLOCK_LEGS, // Block attacks, but with your legs
TEC_WBLOCK_1,	// Weapon block, poor chance -- e.g. pole
TEC_WBLOCK_2,	// Weapon block, moderate chance -- weapon made for blocking
TEC_WBLOCK_3,	// Weapon block, good chance -- shield
TEC_COUNTER,	// Counter-attack on a block or dodge
TEC_BREAK,	// Break from a grab
TEC_DEF_THROW,	// Throw an enemy that attacks you
TEC_DEF_DISARM, // Disarm an enemy

NUM_TECHNIQUES
};

enum bigness_property_aspect {
BIGNESS_ENGINE_NULL,         // like a cookie-cutter-cut cookie, this type has no bigness aspect.
BIGNESS_ENGINE_DISPLACEMENT, // combustion engine CC displacement
BIGNESS_KILOWATTS,           // electric motor power
BIGNESS_WHEEL_DIAMETER,      // wheel size in inches, including tire
//BIGNESS_PLATING_THICKNESS, //
NUM_BIGNESS_ASPECTS,
};

struct style_move
{
 std::string name;
 technique_id tech;
 int level;

 style_move(std::string N, technique_id T, int L) :
  name (N), tech (T), level (L) { };

 style_move()
 {
  name = "";
  tech = TEC_NULL;
  level = 0;
 }
};

// Returns the name of a category of ammo (e.g. "shot")
std::string ammo_name(ammotype t);
// Returns the default ammo for a category of ammo (e.g. "itm_00_shot")
itype_id default_ammo(ammotype guntype);

struct itype
{
 int id;		// ID # that matches its place in master itype list
 			// Used for save files; aligns to itype_id above.
 unsigned char rarity;	// How often it's found
 unsigned int  price;	// Its value

 std::string name;	// Proper name
 std::string description;// Flavor text

 char sym;		// Symbol on the map
 nc_color color;	// Color on the map (color.h)

 material m1;		// Main material
 material m2;		// Secondary material -- MNULL if made of just 1 thing

 unsigned int volume;	// Space taken up by this item
 unsigned int weight;	// Weight in quarter-pounds; is 64 lbs max ok?
 			// Also assumes positive weight.  No helium, guys!
 bigness_property_aspect bigness_aspect;

 signed char melee_dam;	// Bonus for melee damage; may be a penalty
 signed char melee_cut;	// Cutting damage in melee
 signed char m_to_hit;	// To-hit bonus for melee combat; -5 to 5 is reasonable

 unsigned item_flags : NUM_ITEM_FLAGS;
 unsigned techniques : NUM_TECHNIQUES;

 virtual bool is_food()          { return false; }
 virtual bool is_ammo()          { return false; }
 virtual bool is_gun()           { return false; }
 virtual bool is_gunmod()        { return false; }
 virtual bool is_bionic()        { return false; }
 virtual bool is_armor()         { return false; }
 virtual bool is_book()          { return false; }
 virtual bool is_tool()          { return false; }
 virtual bool is_container()     { return false; }
 virtual bool is_software()      { return false; }
 virtual bool is_macguffin()     { return false; }
 virtual bool is_style()         { return false; }
 virtual bool is_artifact()      { return false; }
 virtual bool is_var_veh_part()  { return false; }
 virtual bool is_engine()         { return false; }
 virtual bool is_wheel()          { return false; }
 virtual bool count_by_charges() { return false; }
 virtual std::string save_data() { return std::string(); }

 itype() {
  id = 0;
  rarity = 0;
  name  = "none";
  sym = '#';
  color = c_white;
  m1 = MNULL;
  m2 = MNULL;
  volume = 0;
  weight = 0;
  melee_dam = 0;
  m_to_hit = 0;
  item_flags = 0;
  techniques = 0;
 }

 itype(int pid, unsigned char prarity, unsigned int pprice,
       std::string pname, std::string pdes,
       char psym, nc_color pcolor, material pm1, material pm2,
       unsigned short pvolume, unsigned short pweight,
       signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
       unsigned pitem_flags, unsigned ptechniques = 0) {
  id          = pid;
  rarity      = prarity;
  price       = pprice;
  name        = pname;
  description = pdes;
  sym         = psym;
  color       = pcolor;
  m1          = pm1;
  m2          = pm2;
  volume      = pvolume;
  weight      = pweight;
  melee_dam   = pmelee_dam;
  melee_cut   = pmelee_cut;
  m_to_hit    = pm_to_hit;
  item_flags  = pitem_flags;
  techniques  = ptechniques;
 }
};

// Includes food drink and drugs
struct it_comest : public itype
{
 signed char quench;	// Many things make you thirstier!
 unsigned char nutr;	// Nutrition imparted
 unsigned char spoils;	// How long it takes to spoil (hours / 600 turns)
 unsigned char addict;	// Addictiveness potential
 unsigned char charges;	// Defaults # of charges (drugs, loaf of bread? etc)
 signed char stim;
 signed char healthy;

 signed char fun;	// How fun its use is

 itype_id container;	// The container it comes in
 itype_id tool;		// Tool needed to consume (e.g. lighter for cigarettes)

 virtual bool is_food() { return true; }
// virtual bool count_by_charges() { return charges >= 1 ; }

 virtual bool count_by_charges()
 {
  if (m1 == LIQUID) return true;
  else
  return charges > 1 ;
 }


 void (iuse::*use)(game *, player *, item *, bool);// Special effects of use
 add_type add;				// Effects of addiction

 it_comest(int pid, unsigned char prarity, unsigned int pprice,
           std::string pname, std::string pdes,
           char psym, nc_color pcolor, material pm1,
           unsigned short pvolume, unsigned short pweight,
           signed char pmelee_dam, signed char pmelee_cut,
           signed char pm_to_hit, unsigned pitem_flags,

           signed char pquench, unsigned char pnutr, signed char pspoils,
           signed char pstim, signed char phealthy, unsigned char paddict,
           unsigned char pcharges, signed char pfun, itype_id pcontainer,
           itype_id ptool, void (iuse::*puse)(game *, player *, item *, bool),
           add_type padd)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, MNULL,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) {
  quench = pquench;
  nutr = pnutr;
  spoils = pspoils;
  stim = pstim;
  healthy = phealthy;
  addict = paddict;
  charges = pcharges;
  fun = pfun;
  container = pcontainer;
  tool = ptool;
  use = puse;
  add = padd;
 }
};

// v6, v8, wankel, etc.
struct it_var_veh_part: public itype
{
 // TODO? geometric mean: nth root of product
 unsigned int min_bigness; //CC's
 unsigned int max_bigness;

 it_var_veh_part(int pid, unsigned char prarity, unsigned int pprice,
        std::string pname, std::string pdes,
        char psym, nc_color pcolor, material pm1, material pm2,
        unsigned short pvolume, unsigned short pweight,
        signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
        unsigned effects,

        unsigned int big_min,
        unsigned int big_max,
        bigness_property_aspect big_aspect)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, 0) {
  min_bigness = big_min;
  max_bigness = big_max;
  bigness_aspect = big_aspect;
 }
 virtual bool is_var_veh_part(){return true;}
 virtual bool is_wheel()          { return false; }
 virtual bool is_engine() {
  if (id < itm_1cyl_combustion) return false;
  if (id > itm_v8_combustion) return false;
  return true;
 }
};


struct it_ammo : public itype
{
 ammotype type;		// Enum of varieties (e.g. 9mm, shot, etc)
 unsigned char damage;	// Average damage done
 unsigned char pierce;	// Armor piercing; static reduction in armor
 unsigned char range;	// Maximum range
 signed char accuracy;	// Accuracy (low is good)
 unsigned char recoil;	// Recoil; modified by strength
 unsigned char count;	// Default charges

 unsigned ammo_effects : NUM_AMMO_EFFECTS;

 virtual bool is_ammo() { return true; }
// virtual bool count_by_charges() { return id != itm_gasoline; }
 virtual bool count_by_charges() { return true; }

 it_ammo(int pid, unsigned char prarity, unsigned int pprice,
        std::string pname, std::string pdes,
        char psym, nc_color pcolor, material pm1,
        unsigned short pvolume, unsigned short pweight,
        signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
        unsigned effects,

        ammotype ptype, unsigned char pdamage, unsigned char ppierce,
	signed char paccuracy, unsigned char precoil, unsigned char prange,
        unsigned char pcount)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, MNULL,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, 0) {
  type = ptype;
  damage = pdamage;
  pierce = ppierce;
  range = prange;
  accuracy = paccuracy;
  recoil = precoil;
  count = pcount;
  ammo_effects = effects;
 }
};

struct it_gun : public itype
{
 ammotype ammo;
 Skill *skill_used;
 signed char dmg_bonus;
 signed char accuracy;
 signed char recoil;
 signed char durability;
 unsigned char burst;
 int clip;
 int reload_time;

 virtual bool is_gun() { return true; }

 it_gun(int pid, unsigned char prarity, unsigned int pprice,
        std::string pname, std::string pdes,
        char psym, nc_color pcolor, material pm1, material pm2,
        unsigned short pvolume, unsigned short pweight,
        signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
        unsigned pitem_flags,

	const char *pskill_used, ammotype pammo, signed char pdmg_bonus,
	signed char paccuracy, signed char precoil, unsigned char pdurability,
        unsigned char pburst, int pclip, int preload_time)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) {
  skill_used = pskill_used?Skill::skill(pskill_used):NULL;
  ammo = pammo;
  dmg_bonus = pdmg_bonus;
  accuracy = paccuracy;
  recoil = precoil;
  durability = pdurability;
  burst = pburst;
  clip = pclip;
  reload_time = preload_time;
 }
};

struct it_gunmod : public itype
{
 signed char accuracy, damage, loudness, clip, recoil, burst;
 ammotype newtype;
 unsigned acceptible_ammo_types : NUM_AMMO_TYPES;
 bool used_on_pistol;
 bool used_on_shotgun;
 bool used_on_smg;
 bool used_on_rifle;

 virtual bool is_gunmod() { return true; }

 it_gunmod(int pid, unsigned char prarity, unsigned int pprice,
           std::string pname, std::string pdes,
           char psym, nc_color pcolor, material pm1, material pm2,
           unsigned short pvolume, unsigned short pweight,
           signed char pmelee_dam, signed char pmelee_cut,
           signed char pm_to_hit, unsigned pitem_flags,

           signed char paccuracy, signed char pdamage, signed char ploudness,
           signed char pclip, signed char precoil, signed char pburst,
           ammotype pnewtype, long a_a_t, bool pistol,
           bool shotgun, bool smg, bool rifle)

 :itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
        pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) {
  accuracy = paccuracy;
  damage = pdamage;
  loudness = ploudness;
  clip = pclip;
  recoil = precoil;
  burst = pburst;
  newtype = pnewtype;
  acceptible_ammo_types = a_a_t;
  used_on_pistol = pistol;
  used_on_shotgun = shotgun;
  used_on_smg = smg;
  used_on_rifle = rifle;
 }
};

struct it_armor : public itype
{
 unsigned char covers; // Bitfield of enum body_part
 signed char encumber;
 unsigned char dmg_resist;
 unsigned char cut_resist;
 unsigned char env_resist; // Resistance to environmental effects
 signed char warmth;
 unsigned char storage;

 virtual bool is_armor() { return true; }
 virtual bool is_artifact() { return false; }
 virtual std::string save_data() { return std::string(); }

 it_armor()
 {
  covers = 0;
  encumber = 0;
  dmg_resist = 0;
  cut_resist = 0;
  env_resist = 0;
  warmth = 0;
  storage = 0;
 }

 it_armor(int pid, unsigned char prarity, unsigned int pprice,
          std::string pname, std::string pdes,
          char psym, nc_color pcolor, material pm1, material pm2,
          unsigned short pvolume, unsigned short pweight,
          signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
          unsigned pitem_flags,

          unsigned char pcovers, signed char pencumber,
          unsigned char pdmg_resist, unsigned char pcut_resist,
          unsigned char penv_resist, signed char pwarmth,
          unsigned char pstorage)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) {
  covers = pcovers;
  encumber = pencumber;
  dmg_resist = pdmg_resist;
  cut_resist = pcut_resist;
  env_resist = penv_resist;
  warmth = pwarmth;
  storage = pstorage;
 }
};

struct it_book : public itype
{
 Skill *type;		// Which skill it upgrades
 unsigned char level;	// The value it takes the skill to
 unsigned char req;	// The skill level required to understand it
 signed char fun;	// How fun reading this is
 unsigned char intel;	// Intelligence required to read, at all
 unsigned char time;	// How long, in 10-turns (aka minutes), it takes to read
			// "To read" means getting 1 skill point, not all of em
 virtual bool is_book() { return true; }
 it_book(int pid, unsigned char prarity, unsigned int pprice,
         std::string pname, std::string pdes,
         char psym, nc_color pcolor, material pm1, material pm2,
         unsigned short pvolume, unsigned short pweight,
         signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
         unsigned pitem_flags,

	 const char *ptype, unsigned char plevel, unsigned char preq,
	 signed char pfun, unsigned char pintel, unsigned char ptime)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) {
  type = ptype?Skill::skill(ptype):NULL;
  level = plevel;
  req = preq;
  fun = pfun;
  intel = pintel;
  time = ptime;
 }
};

enum container_flags {
 con_rigid,
 con_wtight,
 con_seals,
 num_con_flags
};

struct it_container : public itype
{
 unsigned char contains;	// Internal volume
 unsigned flags : num_con_flags;
 virtual bool is_container() { return true; }
 it_container(int pid, unsigned char prarity, unsigned int pprice,
              std::string pname, std::string pdes,
              char psym, nc_color pcolor, material pm1, material pm2,
              unsigned short pvolume, unsigned short pweight,
              signed char pmelee_dam, signed char pmelee_cut,
              signed char pm_to_hit, unsigned pitem_flags,

              unsigned char pcontains, unsigned pflags)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) {
  contains = pcontains;
  flags = pflags;
 }
};

struct it_tool : public itype
{
 ammotype ammo;
 unsigned int max_charges;
 unsigned int def_charges;
 unsigned char charges_per_use;
 unsigned char turns_per_charge;
 itype_id revert_to;
 void (iuse::*use)(game *, player *, item *, bool);

 virtual bool is_tool()          { return true; }
 virtual bool is_artifact()      { return false; }
 virtual std::string save_data() { return std::string(); }

 it_tool()
 {
  ammo = AT_NULL;
  max_charges = 0;
  def_charges = 0;
  charges_per_use = 0;
  turns_per_charge = 0;
  revert_to = itm_null;
  use = &iuse::none;
 }

 it_tool(int pid, unsigned char prarity, unsigned int pprice,
         std::string pname, std::string pdes,
         char psym, nc_color pcolor, material pm1, material pm2,
         unsigned short pvolume, unsigned short pweight,
         signed char pmelee_dam, signed char pmelee_cut, signed char pm_to_hit,
         unsigned pitem_flags,

         unsigned int pmax_charges, unsigned int pdef_charges,
         unsigned char pcharges_per_use, unsigned char pturns_per_charge,
         ammotype pammo, itype_id prevert_to,
	 void (iuse::*puse)(game *, player *, item *, bool))
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) {
  max_charges = pmax_charges;
  def_charges = pdef_charges;
  ammo = pammo;
  charges_per_use = pcharges_per_use;
  turns_per_charge = pturns_per_charge;
  revert_to = prevert_to;
  use = puse;
 }
};

struct it_bionic : public itype
{
 std::vector<bionic_id> options;
 int difficulty;

 virtual bool is_bionic()    { return true; }

 it_bionic(int pid, unsigned char prarity, unsigned int pprice,
           std::string pname, std::string pdes,
           char psym, nc_color pcolor, material pm1, material pm2,
           unsigned short pvolume, unsigned short pweight,
           signed char pmelee_dam, signed char pmelee_cut,
           signed char pm_to_hit, unsigned pitem_flags,

           int pdifficulty, ...)
 :itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
        pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) {
   difficulty = pdifficulty;
   va_list ap;
   va_start(ap, pdifficulty);
   bionic_id tmp;
   while ((tmp = (bionic_id)va_arg(ap, int)))
    options.push_back(tmp);
   va_end(ap);
 }
};

struct it_macguffin : public itype
{
 bool readable; // If true, activated with 'R'
 void (iuse::*use)(game *, player *, item *, bool);

 virtual bool is_macguffin() { return true; }

 it_macguffin(int pid, unsigned char prarity, unsigned int pprice,
              std::string pname, std::string pdes,
              char psym, nc_color pcolor, material pm1, material pm2,
              unsigned short pvolume, unsigned short pweight,
              signed char pmelee_dam, signed char pmelee_cut,
              signed char pm_to_hit, unsigned pitem_flags,

              bool preadable,
              void (iuse::*puse)(game *, player *, item *, bool))
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) {
  readable = preadable;
  use = puse;
 }
};

struct it_software : public itype
{
 software_type swtype;
 int power;

 virtual bool is_software()      { return true; }

 it_software(int pid, unsigned char prarity, unsigned int pprice,
             std::string pname, std::string pdes,
             char psym, nc_color pcolor, material pm1, material pm2,
             unsigned short pvolume, unsigned short pweight,
             signed char pmelee_dam, signed char pmelee_cut,
             signed char pm_to_hit, unsigned pitem_flags,

             software_type pswtype, int ppower)
:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) {
  swtype = pswtype;
  power = ppower;
 }
};

struct it_style : public itype
{
 virtual bool is_style()         { return true; }

 std::vector<style_move> moves;

 it_style(int pid, unsigned char prarity, unsigned int pprice,
          std::string pname, std::string pdes,
          char psym, nc_color pcolor, material pm1, material pm2,
          unsigned char pvolume, unsigned char pweight,
          signed char pmelee_dam, signed char pmelee_cut,
          signed char pm_to_hit, unsigned pitem_flags)

:itype(pid, prarity, pprice, pname, pdes, psym, pcolor, pm1, pm2,
       pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags) { }
};

struct it_artifact_tool : public it_tool
{
 art_charge charge_type;
 std::vector<art_effect_passive> effects_wielded;
 std::vector<art_effect_active>  effects_activated;
 std::vector<art_effect_passive> effects_carried;

 virtual bool is_artifact()  { return true; }
 virtual std::string save_data()
 {
  std::stringstream data;
  data << "T " << price << " " << sym << " " << color_to_int(color) << " " <<
          int(m1) << " " << int(m2) << " " << int(volume) << " " <<
          int(weight) << " " << int(melee_dam) << " " << int(melee_cut) <<
          " " << int(m_to_hit) << " " << int(item_flags) << " " <<
          int(charge_type) << " " << max_charges << " " <<
          effects_wielded.size();
  for (int i = 0; i < effects_wielded.size(); i++)
   data << " " << int(effects_wielded[i]);

  data << " " << effects_activated.size();
  for (int i = 0; i < effects_activated.size(); i++)
   data << " " << int(effects_activated[i]);

  data << " " << effects_carried.size();
  for (int i = 0; i < effects_carried.size(); i++)
   data << " " << int(effects_carried[i]);

  data << " " << name << " - ";
  std::string desctmp = description;
  size_t endline;
  do {
   endline = desctmp.find("\n");
   if (endline != std::string::npos)
    desctmp.replace(endline, 1, " = ");
  } while (endline != std::string::npos);
  data << desctmp << " -";
  return data.str();
 }

 it_artifact_tool() {
  ammo = AT_NULL;
  price = 0;
  def_charges = 0;
  charges_per_use = 1;
  turns_per_charge = 0;
  revert_to = itm_null;
  use = &iuse::artifact;
 };

 it_artifact_tool(int pid, unsigned int pprice, std::string pname,
                  std::string pdes, char psym, nc_color pcolor, material pm1,
                  material pm2, unsigned short pvolume, unsigned short pweight,
                  signed char pmelee_dam, signed char pmelee_cut,
                  signed char pm_to_hit, unsigned pitem_flags)

:it_tool(pid, 0, pprice, pname, pdes, psym, pcolor, pm1, pm2,
         pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags,
         0, 0, 1, 0, AT_NULL, itm_null, &iuse::artifact) { };
};

struct it_artifact_armor : public it_armor
{
 std::vector<art_effect_passive> effects_worn;

 virtual bool is_artifact()  { return true; }

 virtual std::string save_data()
 {
  std::stringstream data;
  data << "A " << price << " " << sym << " " << color_to_int(color) << " " <<
          int(m1) << " " << int(m2) << " " << int(volume) << " " <<
          int(weight) << " " << int(melee_dam) << " " << int(melee_cut) <<
          " " << int(m_to_hit) << " " << int(item_flags) << " " <<
          int(covers) << " " << int(encumber) << " " << int(dmg_resist) <<
          " " << int(cut_resist) << " " << int(env_resist) << " " <<
          int(warmth) << " " << int(storage) << " " << effects_worn.size();
  for (int i = 0; i < effects_worn.size(); i++)
   data << " " << int(effects_worn[i]);

  data << " " << name << " - ";
  std::string desctmp = description;
  size_t endline;
  do {
   endline = desctmp.find("\n");
   if (endline != std::string::npos)
    desctmp.replace(endline, 1, " = ");
  } while (endline != std::string::npos);

  data << desctmp << " -";

  return data.str();
 }

 it_artifact_armor()
 {
  price = 0;
 };

 it_artifact_armor(int pid, unsigned int pprice, std::string pname,
                   std::string pdes, char psym, nc_color pcolor, material pm1,
                   material pm2, unsigned short pvolume, unsigned short pweight,
                   signed char pmelee_dam, signed char pmelee_cut,
                   signed char pm_to_hit, unsigned pitem_flags,

                   unsigned char pcovers, signed char pencumber,
                   unsigned char pdmg_resist, unsigned char pcut_resist,
                   unsigned char penv_resist, signed char pwarmth,
                   unsigned char pstorage)
:it_armor(pid, 0, pprice, pname, pdes, psym, pcolor, pm1, pm2,
          pvolume, pweight, pmelee_dam, pmelee_cut, pm_to_hit, pitem_flags,
          pcovers, pencumber, pdmg_resist, pcut_resist, penv_resist, pwarmth,
          pstorage) { };
};

#endif
