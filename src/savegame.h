#ifndef SAVEGAME_H_
#define SAVEGAME_H_

#include <string>
#include <map>
void init_savedata_translation_tables();
extern std::map<std::string, int> monster_ints;
#include "enums.h"
const std::string legacy_mon_id[126] = {"mon_null", "mon_squirrel", "mon_rabbit", "mon_deer",
    "mon_moose", "mon_wolf", "mon_coyote", "mon_bear", "mon_cougar", "mon_crow", "mon_dog",
    "mon_cat", "mon_ant_larva", "mon_ant", "mon_ant_soldier", "mon_ant_queen", "mon_ant_fungus",
    "mon_fly", "mon_bee", "mon_wasp", "mon_graboid", "mon_worm", "mon_halfworm", "mon_sludge_crawler", "mon_zombie",
    "mon_zombie_cop", "mon_zombie_shrieker", "mon_zombie_spitter", "mon_zombie_electric",
    "mon_zombie_smoker", "mon_zombie_swimmer", "mon_zombie_fast", "mon_zombie_brute",
    "mon_zombie_hulk", "mon_zombie_fungus", "mon_boomer", "mon_boomer_fungus", "mon_skeleton",
    "mon_zombie_necro", "mon_zombie_scientist", "mon_zombie_soldier", "mon_zombie_grabber",
    "mon_zombie_master", "mon_beekeeper", "mon_zombie_child", "mon_jabberwock", "mon_triffid", "mon_triffid_young",
    "mon_triffid_queen", "mon_creeper_hub", "mon_creeper_vine", "mon_biollante", "mon_vinebeast",
    "mon_triffid_heart", "mon_fungaloid", "mon_fungaloid_dormant", "mon_fungaloid_young",
    "mon_spore", "mon_fungaloid_queen", "mon_fungal_wall", "mon_blob", "mon_blob_small",
    "mon_chud", "mon_one_eye", "mon_crawler", "mon_sewer_fish", "mon_sewer_snake", "mon_sewer_rat",
    "mon_rat_king", "mon_mosquito", "mon_dragonfly", "mon_centipede", "mon_frog", "mon_slug",
    "mon_dermatik_larva", "mon_dermatik", "mon_spider_wolf", "mon_spider_web",
    "mon_spider_jumping", "mon_spider_trapdoor", "mon_spider_widow", "mon_dark_wyrm",
    "mon_amigara_horror", "mon_dog_thing", "mon_headless_dog_thing", "mon_thing",
    "mon_human_snail", "mon_twisted_body", "mon_vortex", "mon_flying_polyp", "mon_hunting_horror",
    "mon_mi_go", "mon_yugg", "mon_gelatin", "mon_flaming_eye", "mon_kreck", "mon_gracke",
    "mon_blank", "mon_gozu", "mon_shadow", "mon_breather_hub", "mon_breather", "mon_shadow_snake",
    "mon_dementia", "mon_homunculus", "mon_blood_sacrifice", "mon_flesh_angel",
    "mon_eyebot", "mon_manhack", "mon_skitterbot", "mon_secubot", "mon_hazmatbot", "mon_copbot", "mon_molebot",
    "mon_tripod", "mon_chickenbot", "mon_tankbot", "mon_turret", "mon_exploder", "mon_hallu_mom",
    "mon_generator",
// added post 0.8
    "mon_turkey", "mon_raccoon", "mon_opossum", "mon_rattlesnake", "mon_giant_crayfish"

};
const int num_legacy_ter = 174;
const std::string legacy_ter_id[174] = {"t_null", "t_hole", "t_dirt", "t_sand", "t_dirtmound", 
    "t_pit_shallow", "t_pit", "t_pit_corpsed", "t_pit_covered", "t_pit_spiked", "t_pit_spiked_covered", 
    "t_rock_floor", "t_rubble", "t_ash", "t_metal", "t_wreckage", "t_grass", "t_metal_floor", 
    "t_pavement", "t_pavement_y", "t_sidewalk", "t_concrete", "t_floor", "t_dirtfloor", "t_grate", 
    "t_slime", "t_bridge", "t_skylight", "t_emergency_light_flicker", "t_emergency_light", 
    "t_wall_log_half", "t_wall_log", "t_wall_log_chipped", "t_wall_log_broken", "t_palisade", 
    "t_palisade_gate", "t_palisade_gate_o", "t_wall_half", "t_wall_wood", "t_wall_wood_chipped", 
    "t_wall_wood_broken", "t_wall_v", "t_wall_h", "t_concrete_v", "t_concrete_h", "t_wall_metal_v", 
    "t_wall_metal_h", "t_wall_glass_v", "t_wall_glass_h", "t_wall_glass_v_alarm", 
    "t_wall_glass_h_alarm", "t_reinforced_glass_v", "t_reinforced_glass_h", "t_bars", "t_door_c", 
    "t_door_b", "t_door_o", "t_door_locked_interior", "t_door_locked", "t_door_locked_alarm", 
    "t_door_frame", "t_chaingate_l", "t_fencegate_c", "t_fencegate_o", "t_chaingate_c", 
    "t_chaingate_o", "t_door_boarded", "t_door_metal_c", "t_door_metal_o", "t_door_metal_locked", 
    "t_door_bar_c", "t_door_bar_o", "t_door_bar_locked", "t_door_glass_c", "t_door_glass_o", 
    "t_portcullis", "t_recycler", "t_window", "t_window_taped", "t_window_domestic", 
    "t_window_domestic_taped", "t_window_open", "t_curtains", "t_window_alarm", "t_window_alarm_taped", 
    "t_window_empty", "t_window_frame", "t_window_boarded", "t_window_stained_green", 
    "t_window_stained_red", "t_window_stained_blue", "t_rock", "t_fault", "t_paper", "t_tree", 
    "t_tree_young", "t_tree_apple", "t_underbrush", "t_shrub", "t_shrub_blueberry", 
    "t_shrub_strawberry", "t_trunk", "t_root_wall", "t_wax", "t_floor_wax", "t_fence_v", "t_fence_h", 
    "t_chainfence_v", "t_chainfence_h", "t_chainfence_posts", "t_fence_post", "t_fence_wire", 
    "t_fence_barbed", "t_fence_rope", "t_railing_v", "t_railing_h", "t_marloss", "t_fungus", 
    "t_tree_fungal", "t_water_sh", "t_water_dp", "t_water_pool", "t_sewage", "t_lava", "t_sandbox", 
    "t_slide", "t_monkey_bars", "t_backboard", "t_gas_pump", "t_gas_pump_smashed", 
    "t_generator_broken", "t_missile", "t_missile_exploded", "t_radio_tower", "t_radio_controls", 
    "t_console_broken", "t_console", "t_gates_mech_control", "t_gates_control_concrete", "t_barndoor", 
    "t_palisade_pulley", "t_sewage_pipe", "t_sewage_pump", "t_centrifuge", "t_column", "t_vat", 
    "t_stairs_down", "t_stairs_up", "t_manhole", "t_ladder_up", "t_ladder_down", "t_slope_down", 
    "t_slope_up", "t_rope_up", "t_manhole_cover", "t_card_science", "t_card_military", 
    "t_card_reader_broken", "t_slot_machine", "t_elevator_control", "t_elevator_control_off", 
    "t_elevator", "t_pedestal_wyrm", "t_pedestal_temple", "t_rock_red", "t_rock_green", "t_rock_blue", 
    "t_floor_red", "t_floor_green", "t_floor_blue", "t_switch_rg", "t_switch_gb", "t_switch_rb", 
    "t_switch_even"
};

const int num_legacy_furn = 52;
const std::string legacy_furn_id[52] = {"f_null", "f_hay", "f_bulletin", "f_indoor_plant", "f_bed", 
    "f_toilet", "f_makeshift_bed", "f_sink", "f_oven", "f_woodstove", "f_fireplace", "f_bathtub", 
    "f_chair", "f_armchair", "f_sofa", "f_cupboard", "f_trashcan", "f_desk", "f_exercise", "f_bench", 
    "f_table", "f_pool_table", "f_counter", "f_fridge", "f_glass_fridge", "f_dresser", "f_locker", 
    "f_rack", "f_bookcase", "f_washer", "f_dryer", "f_dumpster", "f_dive_block", "f_crate_c", 
    "f_crate_o", "f_canvas_wall", "f_canvas_door", "f_canvas_door_o", "f_groundsheet", 
    "f_fema_groundsheet", "f_skin_wall", "f_skin_door", "f_skin_door_o", "f_skin_groundsheet", 
    "f_mutpoppy", "f_safe_c", "f_safe_l", "f_safe_o", "f_plant_seed", "f_plant_seedling", 
    "f_plant_mature", "f_plant_harvest"
};
const std::string obj_type_name[11]={ "OBJECT_NONE", "OBJECT_ITEM", "OBJECT_ACTOR", "OBJECT_PLAYER", "OBJECT_NPC",
    "OBJECT_MONSTER", "OBJECT_VEHICLE", "OBJECT_TRAP", "OBJECT_FIELD", "OBJECT_TERRAIN", "OBJECT_FURNITURE"
};

extern std::map<std::string, int> obj_type_id;
#endif
