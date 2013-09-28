#ifndef SAVEGAME_H_
#define SAVEGAME_H_

#include <string>
#include <map>
void init_savedata_translation_tables();
extern std::map<std::string, int> monster_ints;

const std::string legacy_mon_id[120] = {"mon_null", "mon_squirrel", "mon_rabbit", "mon_deer",
    "mon_moose", "mon_wolf", "mon_coyote", "mon_bear", "mon_cougar", "mon_crow", "mon_dog",
    "mon_cat", "mon_ant_larva", "mon_ant", "mon_ant_soldier", "mon_ant_queen", "mon_ant_fungus",
    "mon_fly", "mon_bee", "mon_wasp", "mon_graboid", "mon_worm", "mon_halfworm", "mon_zombie",
    "mon_zombie_cop", "mon_zombie_shrieker", "mon_zombie_spitter", "mon_zombie_electric",
    "mon_zombie_smoker", "mon_zombie_swimmer", "mon_zombie_fast", "mon_zombie_brute",
    "mon_zombie_hulk", "mon_zombie_fungus", "mon_boomer", "mon_boomer_fungus", "mon_skeleton",
    "mon_zombie_necro", "mon_zombie_scientist", "mon_zombie_soldier", "mon_zombie_grabber",
    "mon_zombie_master", "mon_beekeeper", "mon_zombie_child", "mon_triffid", "mon_triffid_young",
    "mon_triffid_queen", "mon_creeper_hub", "mon_creeper_vine", "mon_biollante", "mon_vinebeast",
    "mon_triffid_heart", "mon_fungaloid", "mon_fungaloid_dormant", "mon_fungaloid_young",
    "mon_spore", "mon_fungaloid_queen", "mon_fungal_wall", "mon_blob", "mon_blob_small",
    "mon_chud", "mon_one_eye", "mon_crawler", "mon_sewer_fish", "mon_sewer_snake", "mon_sewer_rat",
    "mon_rat_king", "mon_mosquito", "mon_dragonfly", "mon_centipede", "mon_frog", "mon_slug",
    "mon_dermatik_larva", "mon_dermatik", "mon_jabberwock", "mon_spider_wolf", "mon_spider_web",
    "mon_spider_jumping", "mon_spider_trapdoor", "mon_spider_widow", "mon_dark_wyrm",
    "mon_amigara_horror", "mon_dog_thing", "mon_headless_dog_thing", "mon_thing",
    "mon_human_snail", "mon_twisted_body", "mon_vortex", "mon_flying_polyp", "mon_hunting_horror",
    "mon_mi_go", "mon_yugg", "mon_gelatin", "mon_flaming_eye", "mon_kreck", "mon_gracke",
    "mon_blank", "mon_gozu", "mon_shadow", "mon_breather_hub", "mon_breather", "mon_shadow_snake",
    "mon_eyebot", "mon_manhack", "mon_skitterbot", "mon_secubot", "mon_copbot", "mon_molebot",
    "mon_tripod", "mon_chickenbot", "mon_tankbot", "mon_turret", "mon_exploder", "mon_hallu_mom",
    "mon_generator",
// added post 0.8
    "mon_turkey", "mon_raccoon", "mon_opossum", "mon_rattlesnake", "mon_giant_crayfish"

};
#endif
