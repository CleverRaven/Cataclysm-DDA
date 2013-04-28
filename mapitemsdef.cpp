#include "game.h"
#include "itype.h"
#include "omdata.h"
#include "setvector.h"
#include <cstdarg>

void game::init_mapitems()
{
 setvector(mapitems[mi_child_items],
  "chocolate", "candy", "crack", "bat", "backpack", "pockknife", "mag_comic", NULL);

 setvector(
   mapitems[mi_field],
	"rock", "strawberries", NULL);

 setvector(
   mapitems[mi_forest],
	"rock", "stick", "mushroom", "mushroom_poison",
	"mushroom_magic", "blueberries", NULL);

 setvector(
   mapitems[mi_hive],
	"honeycomb", NULL);

 setvector(
   mapitems[mi_hive_center],
	"honeycomb", "royal_jelly", NULL);

 setvector(
   mapitems[mi_road],
	"muffler", "pipe", "motor", "seat",
   "wheel", "wheel_wide", "wheel_bicycle", "wheel_motorbike", "wheel_small",
   "1cyl_combustion", "v2_combustion", "i4_combustion", "v6_combustion",
   "v8_combustion", "foot_crank", NULL);

 setvector(
   mapitems[mi_livingroom],
	"rootbeer", "pizza", "cola", "cig", "cigar", "weed",
	"coke", "meth", "sneakers", "socks", "boots", "boots_winter", "socks_wool",
	"flip_flops", "dress_shoes", "heels", "coat_rain", "poncho",
	"gloves_light", "mittens", "gloves_light", "mittens",
	"armguard_soft", "armguard_hard",
	"gloves_wool", "gloves_winter", "gloves_liner", "gloves_leather",
	"gloves_fingerless", "bandana", "scarf", "hat_cotton",
	"hat_knit", "hat_fur", "helmet_bike", "helmet_motor",
	"mag_tv", "mag_news", "lighter", "matches", "extinguisher", "mp3",
	"usb_drive", NULL);

 setvector(
   mapitems[mi_kitchen],
	"chips", "pot", "pan", "knife_butter", "knife_steak", "knife_butcher",
	"cookbook", "rag", "hotplate", "flashlight", "extinguisher",
	"whiskey", "bleach", "ammonia", "flour", "sugar", "salt",
	"tea_raw", "coffee_raw", "funnel",
	NULL);

 setvector(
   mapitems[mi_knifeblock],
    "knife_steak", "knife_butcher", "knife_combat", "pockknife",
    NULL);

 setvector(
   mapitems[mi_fridge],
	"water_clean", "oj", "cola", "rootbeer", "milk", "V8", "apple",
	"sandwich_t", "mushroom", "blueberries", "strawberries",
	"tomato", "broccoli", "zucchini", "frozen_dinner", "vodka",
	"apple_cider", "jihelucake", NULL);

 setvector(
   mapitems[mi_home_hw],
	"superglue", "string_6", "string_36", "screwdriver", "wrench",
	"hacksaw", "xacto", "gloves_leather", "mask_dust",
	"glasses_safety", "battery", "nail", "nailgun",
  "solar_cell",
	"manual_mechanics", "hammer", "flashlight", "soldering_iron",
	"bubblewrap", "binoculars", "duct_tape", "lawnmower", "foot_crank",
        "boltcutters", "spray_can", NULL);

 setvector(
   mapitems[mi_bedroom],
	"inhaler", "cig", "cigar", "weed", "coke", "meth",
	"heroin", "sneakers", "socks", "mocassins", "bandana", "glasses_eye", "sunglasses",
	"glasses_reading", "hat_ball", "backpack", "purse", "mbag",
	"fanny", "battery", "bb", "bbgun", "mag_porn", "mag_tv", "mag_gaming",
	"mag_news", "novel_romance", "novel_drama", "novel_mystery", "manual_mechanics",
	"manual_speech", "manual_business", "manual_computers",
	"lighter", "matches", "sewing_kit", "thread", "scissors", "soldering_iron",
	"radio", "syringe", "mp3", "usb_drive", "firecracker_pack", "firecracker", 
	"chips", "cola", "picklocks", "wolfsuit", "glowstick", "blanket", "house_coat", NULL);

 setvector(
   mapitems[mi_homeguns],
	"22_lr", "9mm", "crossbow", "sig_mosquito", "sw_22",
	"glock_19", "usp_9mm", "sw_619", "taurus_38", "sig_40",
	"sw_610", "ruger_redhawk", "deagle_44", "usp_45", "m1911",
	"fn57", "mac_10", "shotgun_sawn", "suppressor", "grip",
	"clip", "spare_mag", "grenade", "EMPbomb", "gasbomb", "tazer",
	"longbow", "compbow", "arrow_wood", "arrow_cf", "u_shotgun", "shot_hull",
        "9mm_casing", "38_casing", "40_casing", "44_casing", "45_casing",
        "57mm_casing", "46mm_casing", "762_casing", "223_casing",
        "3006_casing", "308_casing", "gunpowder",
        "shotgun_primer", "smpistol_primer", "lgpistol_primer",
        "smrifle_primer", "lgrifle_primer", "lead", "puller", "press", NULL);

 setvector(
   mapitems[mi_dresser],
	"jeans", "shorts", "pants", "pants_leather", "pants_cargo", "shorts_cargo", "skirt",
  "dress", "tshirt", "polo_shirt", "dress_shirt", "tank_top",
	"sweatshirt", "sweater", "hoodie", "jacket_light",
	"jacket_jean", "blazer", "jacket_leather", "poncho",
	"trenchcoat", "peacoat", "vest", "mag_porn", "lighter",
	"sewing_kit", "thread", "flashlight", "suit", "tophat",
	"glasses_monocle", "duct_tape", "firecracker_pack", "firecracker", "wolfsuit", "snuggie",
    NULL);

 setvector(
   mapitems[mi_dining],
	"wrapper", "knife_butter", "knife_steak", "bottle_glass",
	NULL);

 setvector(
   mapitems[mi_snacks],
	"chips", "pretzels", "chocolate", "jerky", "candy",
	"tea_raw", "coffee_raw", "chips2", "chips3", NULL);

 setvector(
   mapitems[mi_fridgesnacks],
	"water_clean", "oj", "apple_cider", "energy_drink", "cola",
	"rootbeer", "milk", "V8", "sandwich_t", "frozen_dinner",
	"pizza", "pie", NULL);

 setvector(
   mapitems[mi_fast_food],
	"water_clean", "cola", "rootbeer", "sandwich_t", "sandwich_t", "sandwich_t", "pizza",
	"pie", "chips", "candy", NULL);

 setvector(
   mapitems[mi_coffee_shop],
	"water_clean", "cola", "rootbeer", "sandwich_t", "pizza", "pretzels", "chocolate", "jerky", "candy",
	"tea_raw", "pie", "chips", "candy", "coffee_raw", "coffee_raw", "coffee_raw", "coffee_raw", NULL);

 setvector(
   mapitems[mi_behindcounter],
	"aspirin", "caffeine", "cig", "cigar", "battery",
	"shotgun_sawn", "mag_porn", "lighter", "matches", "flashlight",
	"extinguisher", "tazer", "mp3", "roadmap", NULL);

 setvector(
   mapitems[mi_magazines],
	"mag_tv", "mag_news", "mag_cars", "mag_cooking",
	"novel_romance", "novel_spy", "mag_carpentry", "mag_comic",
	"mag_guns", "mag_archery", "novel_horror", "novel_mystery", "novel_fantasy",
	"mag_dodge","mag_comic","mag_throwing", "novel_firstaid",
	"mag_gaming", "Mag_swimming", NULL);

 setvector(
   mapitems[mi_softdrugs],
	"bandages", "1st_aid", "vitamins", "aspirin", "caffeine",
   "pills_sleep", "iodine", "dayquil", "nyquil", "heatpack", NULL);

 setvector(
   mapitems[mi_harddrugs],
	"inhaler", "codeine", "oxycodone", "tramadol", "xanax",
	"adderall", "thorazine", "prozac", "antibiotics", "syringe",
	 NULL);

 setvector(
   mapitems[mi_cannedfood],
	"can_beans", "can_corn", "can_spam", "can_pineapple",
	"can_coconut", "can_sardine", "can_tuna", "can_catfood",
	"broth", "soup_veggy", "soup_meat", "flour", "sugar", "salt", NULL);

 setvector(
   mapitems[mi_pasta],
	"spaghetti_raw", "macaroni_raw", "ravioli", "sauce_red",
	"sauce_pesto", "bread", NULL);

 setvector(
   mapitems[mi_produce],
	"apple", "orange", "lemon", "mushroom", "potato_raw",
	"blueberries", "strawberries", "tomato", "broccoli",
	"zucchini", NULL);

 setvector(
   mapitems[mi_cleaning],
	"salt_water", "bleach", "ammonia", "broom", "mop",
	"gloves_rubber", "mask_dust", "bottle_plastic", "sewing_kit", "thread",
	"rag", "scissors", "string_36", NULL);

 setvector(
   mapitems[mi_hardware],
	"superglue", "chain", "rope_6", "rope_30", "glass_sheet",
	"pipe", "nail", "hose", "string_36", "frame", "metal_tank",
	"wire", "wire_barbed", "duct_tape", "jerrycan", "lawnmower",
  "foot_crank", "spray_can", "ax", "jerrycan_big", "funnel",
	NULL);

 setvector(
   mapitems[mi_tools],
	"screwdriver", "hammer", "wrench", "saw", "hacksaw",
	"hammer_sledge", "xacto", "flashlight", "crowbar", "nailgun",
	"press", "puller", "boltcutters", "ax", NULL);

 setvector(
   mapitems[mi_bigtools],
	"broom", "mop", "hoe", "shovel", "chainsaw_off",
	"hammer_sledge", "jackhammer", "jacqueshammer", "welder", "ax", NULL);

 setvector(
   mapitems[mi_mischw],
	"2x4", "machete", "boots_steel", "hat_hard", "mask_filter",
	"glasses_safety", "bb", "bbgun", "beartrap", "two_way_radio",
	"radio", "hotplate", "extinguisher", "nailgun",
	"manual_mechanics", "manual_carpentry", "mag_carpentry",
	"duct_tape", "lawnmower", "boltcutters",
	"foot_crank", "cable", "textbook_mechanics", NULL);

 setvector(
   mapitems[mi_consumer_electronics],
  "amplifier", "antenna", "battery", "soldering_iron", "solar_cell",
	"screwdriver", "processor", "RAM", "mp3", "flashlight",
	"radio", "hotplate", "receiver", "transponder", "tazer",
	"two_way_radio", "usb_drive", "manual_electronics", "cable", NULL);

 setvector(
   mapitems[mi_sports],
	"bandages", "aspirin", "bat", "bat_metal", "sneakers", "socks", "tshirt",
	"tank_top", "gloves_fingerless", "glasses_safety", "armguard_soft", "armguard_hard",
	"goggles_swim", "goggles_ski", "hat_ball", "helmet_bike",
	"helmet_ball", "manual_brawl", "foot_crank", "glowstick", NULL);

 setvector(
   mapitems[mi_camping],
	"rope_30", "hatchet", "pot", "pan", "binoculars", "firecracker_pack",
	"hotplate", "knife_combat", "machete", "vest", "backpack",
	"bb", "bolt_steel", "bbgun", "crossbow", "manual_knives","manual_survival",
	"manual_first_aid", "manual_traps", "lighter", "matches", "sewing_kit", "thread",
	"hammer", "flashlight", "water_purifier", "radio", "beartrap",
 "UPS_off", "string_36", "longbow", "compbow", "arrow_wood",
 "arrow_cf", "wire", "rollmat", "tent_kit", "canteen", "ax",
 "heatpack", "glowstick", "emer_blanket", "cloak", "sleeping_bag", NULL);


 setvector(
   mapitems[mi_allsporting],
	"aspirin", "bat", "bat_metal", "sneakers", "socks", "tshirt", "tank_top",
	"gloves_fingerless", "glasses_safety", "goggles_swim",
	"armguard_soft", "armguard_hard", "mag_firstaid", "mag_throwing", "mag_swimming",
	"goggles_ski", "hat_ball", "helmet_bike", "helmet_ball",
	"manual_brawl", "rope_30", "hatchet", "pot", "pan",
	"binoculars", "hotplate", "knife_combat", "machete", "vest",
	"backpack", "bb", "bolt_steel", "bbgun", "crossbow",
	"manual_knives", "manual_first_aid", "manual_traps", "lighter", "matches",
	"sewing_kit", "thread", "hammer", "flashlight", "water_purifier",
	"radio", "beartrap", "extinguisher", "string_36", "longbow",
	"compbow", "arrow_wood", "arrow_cf", "rollmat", "tent_kit",
    "foot_crank", "mag_archery", "heatpack", "glowstick", NULL);

 setvector(
   mapitems[mi_alcohol],
  "whiskey", "vodka", "gin", "rum", "tequila", "triple_sec", "beer", NULL);

 setvector(
   mapitems[mi_pool_table],
	"pool_cue", "pool_ball", NULL);

 setvector(
   mapitems[mi_trash],
	"iodine", "meth", "heroin", "wrapper", "string_6", "chain",
	"glass_sheet", "stick", "muffler", "pipe", "bag_plastic",
	"bottle_plastic", "bottle_glass", "can_drink", "can_food",
	"box_small", "bubblewrap", "lighter", "matches", "syringe", "rag",
	"software_hacking", "jug_plastic", "spray_can", "keg", NULL);

 setvector(
   mapitems[mi_ammo],
	"shot_bird", "shot_00", "shot_slug", "22_lr", "22_cb",
	"22_ratshot", "9mm", "9mmP", "9mmP2", "38_special",
	"38_super", "10mm", "40sw", "44magnum", "45_acp", "45_jhp",
	"45_super", "57mm", "46mm", "762_m43", "762_m87", "223",
	"556", "270", "3006", "308", "762_51", NULL);

 setvector(
   mapitems[mi_pistols],
	"sig_mosquito", "sw_22", "glock_19", "usp_9mm", "sw_619",
	"taurus_38", "sig_40", "sw_610", "ruger_redhawk", "deagle_44",
	"usp_45", "m1911", "fn57", "hk_ucp", NULL);

 setvector(
   mapitems[mi_shotguns],
	"shotgun_s", "shotgun_d", "remington_870", "mossberg_500",
	"saiga_12", NULL);

 setvector(
   mapitems[mi_rifles],
	"marlin_9a", "ruger_1022", "browning_blr", "remington_700",
	"sks", "ruger_mini", "savage_111f", NULL);

 setvector(
   mapitems[mi_smg],
	"american_180", "uzi", "tec9", "calico", "hk_mp5", "mac_10",
	"hk_ump45", "TDI", "fn_p90", "hk_mp7", NULL);

 setvector(
   mapitems[mi_assault],
	"hk_g3", "hk_g36", "ak47", "fn_fal", "acr", "ar15",
	"scar_l", "scar_h", "steyr_aug", "m249", NULL);

 setvector(
   mapitems[mi_allguns],
	"sig_mosquito", "sw_22", "glock_19", "usp_9mm", "sw_619",
	"taurus_38", "sig_40", "sw_610", "ruger_redhawk", "deagle_44",
	"usp_45", "m1911", "fn57", "hk_ucp", "shotgun_s",
	"shotgun_d", "remington_870", "mossberg_500", "saiga_12",
	"american_180", "uzi", "tec9", "calico", "hk_mp5", "mac_10",
	"hk_ump45", "TDI", "fn_p90", "hk_mp7", "marlin_9a",
	"ruger_1022", "browning_blr", "remington_700", "sks",
	"ruger_mini", "savage_111f", "hk_g3", "hk_g36", "ak47",
	"fn_fal", "acr", "ar15", "scar_l", "scar_h", "steyr_aug",
	"m249", NULL);

 setvector(
   mapitems[mi_gunxtras],
	"glasses_safety", "goggles_nv", "holster", "bootstrap",
	"mag_guns", "mag_archery", "flashlight", "UPS_off", "suppressor", "grip",
	"barrel_big", "barrel_small", "barrel_rifled", "clip", "spare_mag",
	"clip2", "stablizer", "blowback", "autofire", "retool_45",
	"retool_9mm", "retool_22", "retool_57", "retool_46",
	"retool_308", "retool_223", "tazer","shot_hull",
        "9mm_casing", "38_casing", "40_casing", "44_casing", "45_casing",
        "57mm_casing", "46mm_casing", "762_casing", "223_casing",
        "3006_casing", "308_casing", "gunpowder",
        "shotgun_primer", "smpistol_primer", "lgpistol_primer",
        "smrifle_primer", "lgrifle_primer", "lead", "press", "puller", NULL);

 setvector(
   mapitems[mi_shoes],
	"sneakers", "socks", "boots", "flip_flops", "dress_shoes", "heels",
    NULL);

 setvector(
   mapitems[mi_pants],
	"jeans", "shorts", "pants", "pants_leather", "pants_cargo", "shorts_cargo", "skirt",
	"dress", NULL);

 setvector(
   mapitems[mi_shirts],
	"tshirt", "polo_shirt", "dress_shirt", "tank_top",
	"sweatshirt", "sweater", "hoodie", "under_armor",
    NULL);

 setvector(
   mapitems[mi_jackets],
	"jacket_light", "jacket_jean", "blazer", "jacket_leather",
	"coat_rain", "trenchcoat", NULL);

 setvector(
   mapitems[mi_winter],
	"coat_winter", "peacoat", "gloves_light", "mittens",
	"gloves_wool", "gloves_winter", "gloves_liner", "gloves_leather", "scarf",
	"hat_cotton", "hat_knit", "hat_fur", "pants_ski", "long_underpants",
    "balclava", NULL);

 setvector(
   mapitems[mi_bags],
	"backpack", "purse", "mbag", "rucksack", NULL);

 setvector(
   mapitems[mi_allclothes],
	"jeans", "shorts", "pants", "suit", "tophat", "glasses_monocle",
	"pants_leather", "pants_cargo", "shorts_cargo", "skirt", "tshirt",
	"polo_shirt", "dress_shirt", "tank_top", "sweatshirt",
	"sweater", "hoodie", "jacket_light", "jacket_jean",
	"blazer", "jacket_leather", "coat_winter", "peacoat",
	"gloves_light", "mittens", "gloves_wool", "gloves_winter", "gloves_liner",
	"gloves_leather", "scarf", "hat_cotton", "hat_knit",
	"hat_fur", "UPS_off", "under_armor", "balclava", "pants_ski", "long_underpants",
    "trenchcoat_leather", "cloak", "house_coat", "jedi_cloak", NULL);

 setvector(
   mapitems[mi_novels],
	"novel_romance", "novel_spy", "novel_scifi", "novel_drama",
	"cookbok_human", "novel_mystery", "novel_fantasy", "novel_horror",
	NULL);

 setvector(
   mapitems[mi_manuals],
	"manual_brawl", "manual_knives", "manual_mechanics",
	"manual_speech", "manual_business", "manual_first_aid",
	"manual_computers", "cookbook", "manual_electronics",
	"manual_tailor", "manual_traps", "manual_carpentry",
        "manual_survival", NULL);

 setvector(
   mapitems[mi_textbooks],
	"textbook_computers", "textbook_electronics", "textbook_business",
	"textbook_chemistry", "textbook_carpentry", "SICP",
	"textbook_robots", "textbook_mechanics", NULL);

 setvector(
   mapitems[mi_cop_weapons],
	"baton", "kevlar", "vest", "gloves_leather", "mask_gas",
	"goggles_nv", "helmet_riot", "holster", "bootstrap",
	"armguard_hard",
	"shot_00", "9mm", "usp_9mm", "remington_870", "two_way_radio",
	"UPS_off", "tazer", NULL);

 setvector(
   mapitems[mi_cop_evidence],
	"weed", "coke", "meth", "heroin", "syringe", "electrohack",
	"knife_combat", "crowbar", "tazer", "software_hacking", "spray_can", NULL);

 setvector(
   mapitems[mi_hospital_lab],
	"blood", "iodine", "bleach", "bandages", "syringe",
	"canister_empty", "coat_lab", "gloves_medical", "mask_dust",
	"glasses_safety", "vacutainer", "usb_drive", NULL);

 setvector(
   mapitems[mi_hospital_samples],
	"blood", "vacutainer", NULL);

 setvector(
   mapitems[mi_surgery],
	"blood", "iodine", "bandages", "scalpel", "syringe",
	"gloves_medical", "mask_dust", "vacutainer", "rag_bloody", NULL);

 setvector(
   mapitems[mi_office],
	"cola", "aspirin", "cigar", "glasses_eye", "sunglasses", "glasses_reading",
	"purse", "mbag", "battery", "mag_news", "manual_business",
	"textbook_business", "lighter", "matches", "extinguisher", "flashlight",
	"radio", "bubblewrap", "coffee_raw", "usb_drive",
	"software_useless", NULL);
	
 setvector(
   mapitems[mi_cubical_office],
    "cola", "aspirin", "cigar", "glasses_eye", "sunglasses", "glasses_reading",
	"purse", "mbag", "battery", "mag_news", "manual_business",
	"textbook_business", "manual_computers", "textbook_computers", "lighter", "matches", "extinguisher", "flashlight",
	"radio", "bubblewrap", "coffee_raw", "usb_drive","rootbeer", "cig", "coat_rain", "poncho",
	"mag_tv", "mag_news", "lighter", "matches", "extinguisher", "mp3", NULL);

 setvector(
   mapitems[mi_church],
    "glasses_eye", "sunglasses", "glasses_reading", "lighter", "matches", "coat_rain", "cane", "candlestick",
    "candlestick", "candlestick", "candlestick", NULL);

 setvector(
   mapitems[mi_vault],
	"purifier", "plut_cell", "ftk93", "nx17", "canister_goo",
	"UPS_off", "gold", "plasma_engine",
   "bio_time_freeze", "bio_teleport",
   "power_armor_basic",
  "minireactor", "alloy_plate", NULL);

 setvector(
   mapitems[mi_art],
	"fur", "katana", "petrified_eye", "spiral_stone", "rapier",
	"cane", "candlestick", "heels", "ring", "necklace", NULL);

 setvector(
   mapitems[mi_pawn],
	"cigar", "katana", "gold", "rapier", "cane", "suit",
	"mask_gas", "goggles_welding", "goggles_nv", "glasses_monocle",
	"tophat", "ruger_redhawk", "deagle_44", "m1911", "geiger_off",
	"UPS_off", "tazer", "mp3", "fur", "leather", "string_36",
	"chain", "steel_chunk", "spring", "steel_lump", "manhole_cover", "rock",
	"hammer_sledge", "ax", "knife_butcher", "knife_combat",
	"bat", "petrified_eye", "binoculars", "boots", "mocassins",
	"dress_shoes", "heels", "pants", "pants_army", "skirt",
	"jumpsuit", "dress", "dress_shirt", "sweater", "blazer",
	"jacket_leather", "coat_fur", "peacoat", "coat_lab",
	"helmet_army", "hat_fur", "holster", "bootstrap",
	"remington_870", "browning_blr", "remington_700", "sks",
	"novel_romance", "novel_spy", "novel_scifi", "novel_drama",
	"SICP", "textbook_robots", "extinguisher", "radio",
	"chainsaw_off", "jackhammer", "jacqueshammer", "ring", "necklace", "usb_drive",
	"broadsword", "morningstar", "helmet_plate", "cot", "rollmat", "tent_kit",
    "bat_metal",  "lawnmower",
    "makeshift_machete", "picklocks", "rucksack", "puller", "press", NULL);

 setvector(
   mapitems[mi_mil_surplus], // NOT food or armor!
	"knife_combat", "binoculars", "bolt_steel", "crossbow",
	"mag_guns", "manual_brawl", "manual_knives", "cot",
	"manual_mechanics", "manual_first_aid", "manual_traps",
	"flashlight", "water_purifier", "two_way_radio", "radio",
 "geiger_off", "usb_drive", "canteen", "jerrycan", "rucksack",
 "heatpack", "sleeping_bag", "emer_blanket", NULL);

 setvector(
   mapitems[mi_shelter],
	"water_clean", "soup_veggy", "soup_meat", "chocolate", "ravioli", "can_beans",
	"can_spam", "can_tuna", "coffee_raw", "bandages", "1st_aid",
	"vitamins", "iodine", "dayquil", "screwdriver", "boots",
	"boots_winter", "socks_wool", "jeans", "shorts", "tshirt", "sweatshirt", "sweater",
	"coat_winter", "gloves_wool", "gloves_winter", "gloves_liner", "hat_knit",
	"backpack", "battery", "novel_scifi", "novel_drama", "mag_dodge",
	"manual_first_aid", "manual_tailor", "manual_carpentry",
	"lighter", "matches", "sewing_kit", "thread", "hammer", "extinguisher",
	"flashlight", "hotplate", "water_purifier", "radio", "rollmat",
 "tent_kit", "canteen", "spray_can", "ax", "heatpack", "blanket", "emer_blanket", NULL);

 setvector(
   mapitems[mi_mechanics],
        "wrench", "frame", "motor",
        "wheel", "wheel_wide", "wheel_bicycle", "wheel_motorbike", "wheel_small",
        "1cyl_combustion", "v2_combustion", "i4_combustion", "v6_combustion",
        "vehicle_controls", "v8_combustion", "hacksaw", "welder", "motor",
        "goggles_welding", "solar_cell",
        "motor_large", "storage_battery", "solar_panel", "jerrycan", "jerrycan_big", "metal_tank", NULL);

 setvector(
   mapitems[mi_chemistry],
	"iodine", "water_clean", "salt_water", "bleach", "ammonia",
	"mutagen", "purifier", "royal_jelly", "superglue",
	"bottle_glass", "syringe", "extinguisher", "hotplate",
	"software_medical", "funnel", NULL);

 setvector(
   mapitems[mi_teleport],
	"screwdriver", "wrench", "jumpsuit", "mask_dust",
	"glasses_safety", "goggles_welding", "teleporter", "usb_drive",
	NULL);

 setvector(
   mapitems[mi_goo],
	"jumpsuit", "gloves_rubber", "mask_filter", "glasses_safety",
	"helmet_riot", "lighter", "canister_goo", NULL);

 setvector(
   mapitems[mi_cloning_vat],
	"fetus", "arm", "leg", NULL);

 setvector(
   mapitems[mi_dissection],
	"iodine", "bleach", "bandages", "string_6", "hacksaw",
	"xacto", "knife_butcher", "machete", "gloves_rubber",
	"bag_plastic", "syringe", "rag", "scissors", "rag_bloody", NULL);

 setvector(
   mapitems[mi_hydro],
	"blueberries", "strawberries", "tomato", "broccoli",
	"zucchini", "potato_raw", "corn", "withered", NULL);

 setvector(
   mapitems[mi_electronics],
	"superglue", "electrohack", "processor", "RAM",
	"power_supply", "amplifier", "transponder", "receiver",
  "antenna", "solar_cell",
	"screwdriver", "mask_dust", "glasses_safety", "goggles_welding",
	"battery", "plut_cell", "manual_electronics",
	"textbook_electronics", "soldering_iron", "hotplate", "UPS_off",
	"usb_drive", "software_useless", NULL);

 setvector(
   mapitems[mi_monparts],
	"meat", "veggy", "meat_tainted", "veggy_tainted",
	"royal_jelly", "ant_egg", "bee_sting", "chitin_piece", NULL);

 setvector(
   mapitems[mi_bionics],
   "bio_solar", "bio_batteries", "bio_metabolics",
   "bio_ethanol", "bio_furnace",
   "bio_tools",      "bio_storage",  "bio_flashlight",
   "bio_lighter",      "bio_magnet",
   "bio_memory",  "bio_painkiller", "bio_alarm",
   "bio_ears", "bio_eye_enhancer", "bio_night_vision",
   "bio_infrared", "bio_scent_vision",
   "bio_targeting", "bio_ground_sonar",
   "bio_membrane", "bio_gills",
   "bio_purifier", "bio_climate", "bio_heatsink", "bio_blood_filter",
   "bio_recycler", "bio_digestion", "bio_evap", "bio_water_extractor",
   "bio_face_mask", "bio_scent_mask", "bio_cloak", "bio_fingerhack",
   "bio_carbon", "bio_armor_head", "bio_armor_torso",
   "bio_armor_arms", "bio_armor_legs",
   "bio_shock", "bio_heat_absorb", "bio_claws",
   "bio_nanobots", "bio_blood_anal",
   "bio_ads", "bio_ods",
   "bio_resonator", "bio_hydraulics",
   "bio_time_freeze", "bio_teleport",
   "bio_blaster", "bio_laser", "bio_emp",
   "bio_power_armor_interface",
	NULL);

 setvector(
   mapitems[mi_bionics_common],
	"bio_power_storage",
   "bio_tools",      "bio_storage",  "bio_flashlight",
   "bio_lighter",      "bio_magnet",   "bio_alarm",
   "bio_solar", "bio_batteries", "bio_metabolics",
   "bio_ethanol", "bio_furnace", NULL);

 setvector(
   mapitems[mi_bots],
	"bot_manhack", "bot_turret", NULL);

 setvector(
   mapitems[mi_launchers],
	"40mm_concussive", "40mm_frag", "40mm_incendiary",
	"40mm_teargas", "40mm_smoke", "40mm_flashbang", "m79",
	"m320", "mgl", "m203", "LAW_Packed", NULL);

 setvector(
   mapitems[mi_mil_rifles],
	"556", "556_incendiary", "762_51", "762_51_incendiary",
	"laser_pack", "12mm", "plasma", "m4a1", "scar_l", "scar_h",
	"m249", "ftk93", "nx17", "hk_g80", "plasma_rifle",
	"suppressor", "clip", "spare_mag", "m203", "UPS_off", "u_shotgun",
	NULL);

 setvector(
   mapitems[mi_grenades],
	"grenade", "flashbang", "EMPbomb", "gasbomb", "smokebomb",
	"dynamite", "mininuke", "c4", NULL);

 setvector(
   mapitems[mi_mil_armor],
	"pants_army", "kevlar", "vest", "mask_gas", "goggles_nv",
	"helmet_army", "backpack", "UPS_off", "beltrig", "under_armor",
    "boots", "armguard_hard", "power_armor_basic", "power_armor_frame",
	"helmet_army", "backpack", "UPS_off", "beltrig", NULL);

 setvector(
   mapitems[mi_mil_food],
	"chocolate", "can_beans", "mre_beef", "mre_veggy", "1st_aid",
 "codeine", "antibiotics", "water_clean", "purifier", "heatpack", NULL);

 setvector(
   mapitems[mi_mil_food_nodrugs],
	"chocolate", "can_beans", "mre_beef", "mre_veggy", "1st_aid",
	"water_clean", NULL);

 setvector(
   mapitems[mi_bionics_mil],
	"bio_power_storage", "bio_solar",
   "bio_solar", "bio_batteries", "bio_metabolics",
   "bio_ethanol", "bio_furnace",
   "bio_ears", "bio_eye_enhancer", "bio_night_vision",
   "bio_infrared", "bio_scent_vision",
   "bio_recycler", "bio_digestion", "bio_evap", "bio_water_extractor",
   "bio_carbon", "bio_armor_head", "bio_armor_torso",
   "bio_armor_arms", "bio_armor_legs",
   "bio_targeting", "bio_ground_sonar",
   "bio_face_mask", "bio_scent_mask", "bio_cloak", "bio_fingerhack",
   "bio_nanobots", "bio_blood_anal",
   "bio_ads", "bio_ods",
   "bio_blaster", "bio_laser", "bio_emp",
   "bio_time_freeze", "bio_teleport",
   "bio_power_armor_interface",
	NULL);

 setvector(
   mapitems[mi_weapons],
	"chain", "hammer", "wrench", "hammer_sledge", "hatchet",
	"ax", "knife_combat", "pipe", "bat", "machete", "katana",
	"baton", "tazer", "rapier", "bat_metal", NULL);

 setvector(
   mapitems[mi_survival_armor],
	"boots_steel", "pants_cargo", "shorts_cargo", "pants_army", "jumpsuit",
	"jacket_leather", "kevlar", "vest", "gloves_fingerless",
	"mask_filter", "mask_gas", "goggles_ski", "helmet_skid",
    "armguard_hard", "under_armor", "long_underpants",
	"helmet_ball", "helmet_riot", "helmet_motor", "holster",
	"bootstrap", "UPS_off", "beltrig", "rucksack",
    "emer_blanket", "cloak", NULL);

 setvector(
   mapitems[mi_survival_tools],
	"bandages", "1st_aid", "caffeine", "iodine", "electrohack",
	"string_36", "rope_30", "chain", "binoculars",
	"bottle_plastic", "lighter", "matches", "sewing_kit", "thread", "extinguisher",
	"flashlight", "crowbar", "chainsaw_off", "beartrap",
	"grenade", "EMPbomb", "hotplate", "UPS_off", "canteen", "spray_can",
 "bio_tools", "bio_ethanol", "heatpack", "glowstick", NULL);

 setvector(
   mapitems[mi_sewage_plant],
	"1st_aid", "motor", "hose", "screwdriver", "wrench", "pipe",
	"boots", "jumpsuit", "coat_lab", "gloves_rubber",
	"mask_filter", "glasses_safety", "hat_hard", "extinguisher",
	"flashlight", "water_purifier", "two_way_radio",
   "bio_tools",      "bio_storage",  "bio_flashlight",
   "bio_lighter",      "bio_magnet",
   "bio_purifier", "bio_climate", "bio_heatsink", "bio_blood_filter", NULL);

 setvector(
   mapitems[mi_mine_storage],
	"rock", "coal", NULL);

 setvector(
   mapitems[mi_mine_equipment],
	"water_clean", "1st_aid", "rope_30", "chain", "boots_steel",
	"jumpsuit", "gloves_leather", "mask_filter", "mask_gas",
	"glasses_safety", "goggles_welding", "goggles_nv", "hat_hard",
	"backpack", "battery", "flashlight", "two_way_radio",
	"jackhammer", "jacqueshammer", "dynamite", "UPS_off",
   "bio_tools", "bio_flashlight", "bio_lighter", "bio_magnet",
   "bio_resonator", "bio_hydraulics",
  "jerrycan", "jerrycan_big", NULL);


 setvector(
   mapitems[mi_spiral],
	"spiral_stone", "vortex_stone", NULL);

 setvector(
   mapitems[mi_radio],
	"cola", "caffeine", "cig", "weed", "amplifier",
	"transponder", "receiver", "antenna", "screwdriver",
	"battery", "mag_porn", "mag_tv", "manual_electronics",
	"lighter", "flashlight", "two_way_radio", "radio", "mp3",
  "solar_cell",
	"usb_drive", NULL);

 setvector(
   mapitems[mi_toxic_dump_equipment],
	"1st_aid", "iodine", "canister_empty", "boots_steel",
	"hazmat_suit", "mask_gas", "hat_hard", "textbook_carpentry",
	"extinguisher", "radio", "geiger_off", "UPS_off",
   "bio_purifier", "bio_climate", "bio_heatsink", "bio_blood_filter", NULL);

 setvector(
   mapitems[mi_subway],
	"wrapper", "string_6", "chain", "rock", "pipe",
	"mag_porn", "bottle_plastic", "bottle_glass", "can_drink",
	"can_food", "lighter", "matches", "flashlight", "rag", "crowbar", "spray_can", NULL);

 setvector(
   mapitems[mi_sewer],
	"mutagen", "fetus", "weed", "mag_porn", "rag", NULL);

 setvector(
   mapitems[mi_cavern],
	"rock", "jackhammer", "jacqueshammer", "flashlight", "dynamite", NULL);

 setvector(
   mapitems[mi_spider],
	"corpse", "mutagen", "purifier", "meat", "meat_tainted",
	"arm", "leg", "1st_aid", "codeine", "oxycodone", "weed",
	"coke", "wrapper", "fur", "leather", "id_science",
	"id_military", "rope_30", "stick", "hatchet", "ax",
	"bee_sting", "chitin_piece", "vest", "mask_gas", "goggles_nv",
	"hat_boonie", "helmet_riot", "bolt_steel", "shot_00",
	"762_m87", "556", "556_incendiary", "3006_incendiary",
	"762_51", "762_51_incendiary", "saiga_12", "hk_mp5", "TDI",
	"savage_111f", "sks", "ak47", "m4a1", "steyr_aug", "v29",
	"nx17", "flamethrower", "flashlight", "radio", "geiger_off",
	"teleporter", "canister_goo", "dynamite", "mininuke",
	"bot_manhack", "UPS_off", "bio_power_storage",
   "bio_flashlight", "bio_lighter",
	"arrow_cf", "spray_can", "bio_blaster", NULL);

 setvector(
   mapitems[mi_ant_food],
	"meat", "veggy", "meat_tainted", "veggy_tainted", "apple",
	"orange", "mushroom", "blueberries", "strawberries",
	"tomato", "broccoli", "zucchini", "potato_raw", "honeycomb",
	"royal_jelly", "arm", "leg", "rock", "stick",
   "bio_metabolics", "bio_blaster", NULL);

 setvector(
   mapitems[mi_ant_egg],
	"ant_egg", NULL);	//TODO: More items here?

 setvector(
   mapitems[mi_biollante],
	"biollante_bud", NULL);

 setvector(
   mapitems[mi_bugs],
	"chitin_piece", NULL);

 setvector(
   mapitems[mi_bees],
	"bee_sting", "chitin_piece", NULL);

 setvector(
   mapitems[mi_wasps],
	"wasp_sting", "chitin_piece", NULL);

 setvector(
   mapitems[mi_robots],
  "processor", "RAM", "power_supply", "amplifier", "solar_cell",
	"transponder", "receiver", "antenna", "steel_chunk", "spring",
	"steel_lump", "motor", "battery", "plut_cell", NULL);

 setvector(
   mapitems[mi_eyebot],
     "flashlight", NULL);

 setvector(
   mapitems[mi_manhack],
    "knife_combat", NULL);

 setvector(
   mapitems[mi_skitterbot],
     "tazer", NULL);

 setvector(
   mapitems[mi_secubot],
    "9mm", "steel_plate", NULL);

 setvector(
   mapitems[mi_copbot],
     "baton", "tazer", "alloy_plate", NULL);

 setvector(
   mapitems[mi_molebot],
     "spiked_plate", "hard_plate", NULL);

 setvector(
   mapitems[mi_tripod],
     "flamethrower", "alloy_plate", NULL);

 setvector(
   mapitems[mi_chickenbot],
     "9mm", "alloy_plate", NULL);

 setvector(
   mapitems[mi_tankbot],
     "tazer", "flamethrower", "9mm", "alloy_plate",
     "hard_plate", NULL);

 setvector(
   mapitems[mi_turret],
     "9mm", NULL);

 setvector(
   mapitems[mi_helicopter],
	"chain", "power_supply", "antenna", "steel_chunk", "spring",
	"steel_lump", "frame", "steel_plate", "spiked_plate",
	"hard_plate", "motor", "motor_large", "hose", "pants_army",
	"jumpsuit", "kevlar", "mask_gas", "helmet_army", "battery",
	"plut_cell", "m249", "v8_combustion", "extinguisher",
	"two_way_radio", "radio", "UPS_off", "beltrig",
    "rucksack", "LAW_Packed", NULL);

// TODO: Replace kevlar with the ceramic plate armor
 setvector(
   mapitems[mi_military],
	"water_clean", "mre_beef", "mre_veggy", "bandages", "1st_aid",
	"iodine", "codeine", "cig", "knife_combat", "boots_steel",
	"pants_army", "kevlar", "vest", "gloves_fingerless",
	"mask_gas", "glasses_safety", "goggles_nv", "hat_boonie",
	"armguard_hard",
	"helmet_army", "backpack", "holster", "bootstrap", "9mm",
	"45_acp", "556", "556_incendiary", "762_51",
	"762_51_incendiary", "laser_pack", "40mm_concussive",
	"40mm_frag", "40mm_incendiary", "40mm_teargas", "40mm_smoke",
	"40mm_flashbang", "usp_9mm", "usp_45", "m4a1", "scar_l",
	"scar_h", "m249", "ftk93", "nx17", "m320", "mgl",
	"suppressor", "clip", "lighter", "flashlight", "two_way_radio",
	"landmine", "grenade", "flashbang", "EMPbomb", "gasbomb",
	"smokebomb", "UPS_off", "tazer", "c4", "hk_g80", "12mm",
	"binoculars", "u_shotgun", "beltrig", "power_armor_basic",
    "power_armor_helmet_basic", "power_armor_frame", "spare_mag",
    "canteen", "jerrycan", "rucksack", "heatpack", "LAW_Packed", NULL);

 setvector(
   mapitems[mi_science],
	"water_clean", "bleach", "ammonia", "mutagen", "purifier",
	"iodine", "inhaler", "adderall", "id_science", "electrohack",
	"RAM", "screwdriver", "canister_empty", "coat_lab",
	"gloves_medical", "mask_dust", "mask_filter", "glasses_eye", "sunglasses",
	"glasses_safety", "textbook_computers", "textbook_electronics",
	"textbook_chemistry", "SICP", "textbook_robots",
	"soldering_iron", "geiger_off", "teleporter", "canister_goo",
	"EMPbomb", "pheromone", "portal", "bot_manhack", "UPS_off",
	"tazer", "plasma", "usb_drive",
   "bio_purifier", "bio_climate", "bio_heatsink", "bio_blood_filter",
	"software_useless", "canteen", NULL);

 setvector(
   mapitems[mi_rare],
	"mutagen", "purifier", "royal_jelly", "fetus", "id_science",
	"id_military", "electrohack", "processor", "armor_chitin",
	"plut_cell", "laser_pack", "m249", "v29", "ftk93", "nx17",
	"conversion_battle", "conversion_sniper", "canister_goo",
	"mininuke", "portal", "c4", "12mm", "hk_g80",
 "power_armor_basic", "power_armor_helmet_basic", "power_armor_frame",
	"plasma", "plasma_rifle", NULL);

 setvector(
   mapitems[mi_stash_food],
	"water_clean", "cola", "jerky", "ravioli", "can_beans",
	"can_corn", "can_spam", NULL);

 setvector(
   mapitems[mi_stash_ammo],
	"bolt_steel", "shot_00", "shot_slug", "22_lr", "9mm",
	"38_super", "10mm", "44magnum", "45_acp", "57mm", "46mm",
	"762_m87", "556", "3006", "762_51", "arrow_cf", "press", "puller", NULL);

 setvector(
   mapitems[mi_stash_wood],
	"stick", "ax", "saw", "2x4", "log", NULL);

 setvector(
   mapitems[mi_stash_drugs],
	"pills_sleep", "oxycodone", "xanax", "adderall", "weed",
	"coke", "meth", "heroin", "crack", "crackpipe", NULL);

 setvector(
   mapitems[mi_drugdealer],
	"energy_drink", "whiskey", "jerky", "bandages", "caffeine",
	"oxycodone", "adderall", "cig", "weed", "coke", "meth",
	"heroin", "syringe", "electrohack", "hatchet", "nailboard",
	"knife_combat", "bat", "machete", "katana", "pants_cargo", "shorts_cargo",
	"hoodie", "gloves_fingerless", "backpack", "holster",
	"armguard_soft", "armguard_hard",
	"shot_00", "9mm", "45_acp", "glock_19", "shotgun_sawn",
	"uzi", "tec9", "mac_10", "suppressor", "clip2", "autofire",
	"mag_porn", "lighter", "matches", "crowbar", "pipebomb", "grenade",
	"mininuke", "crack", "crackpipe", "spare_mag", "bio_blaster", NULL);

 setvector(
   mapitems[mi_wreckage],
	"chain", "steel_chunk", "spring", "steel_lump", "frame", "rock", NULL);

 setvector(
   mapitems[mi_npc_hacker],
	"energy_drink", "adderall", "electrohack", "usb_drive",
	"battery", "manual_computers", "textbook_computers",
  "solar_cell",
	"SICP", "soldering_iron", NULL);

// This one kind of an inverted list; what an NPC will NOT carry
 setvector(
   mapitems[mi_trader_avoid],
	"null", "corpse", "fire", "toolset", "meat", "veggy",
	"meat_tainted", "veggy_tainted", "meat_cooked", "veggy_cooked",
	"mushroom_poison", "spaghetti_cooked", "macaroni_cooked",
	"fetus", "arm", "leg", "wrapper", "manhole_cover", "rock",
	"stick", "bag_plastic", "flashlight_on", "radio_on",
	"chainsaw_on", "pipebomb_act", "grenade_act", "flashbang_act",
	"EMPbomb_act", "gasbomb_act", "smokebomb_act", "molotov_lit",
	"dynamite_act", "firecracker_pack_act", "firecracker_act",
	"mininuke_act", "UPS_on", "mp3_on", "c4armed", "apparatus",
	"brazier", "rag_bloody", "candle_lit", "torch_lit",
	"acidbomb_act", NULL);
}
