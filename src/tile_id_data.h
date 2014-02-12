#ifndef TILE_ID_DATA
#define TILE_ID_DATA

#define toString(x) #x
/*
*********************************************************************************
* Whenever the ter_id, trap_id, field_id, mon_id, furn_id, and/or vpart_id enum *
* values are changed a corresponding change MUST be made in here or the tileset *
* support will become offset (screws up later tiles), or if the id is added to  *
* the end it will just not be shown                                             *
*********************************************************************************
*/

const std::string field_names[num_fields] =
{
    toString(fd_null),
    toString(fd_blood),
    toString(fd_bile),
    toString(fd_gibs_flesh),
    toString(fd_gibs_veggy),
    toString(fd_web),
    toString(fd_slime),
    toString(fd_acid),
    toString(fd_sap),
    toString(fd_sludge),
    toString(fd_fire),
    toString(fd_rubble),
    toString(fd_smoke),
    toString(fd_toxic_gas),
    toString(fd_tear_gas),
    toString(fd_nuke_gas),
    toString(fd_gas_vent),
    toString(fd_fire_vent),
    toString(fd_flame_burst),
    toString(fd_electricity),
    toString(fd_fatigue),
    toString(fd_push_items),
    toString(fd_shock_vent),
    toString(fd_acid_vent),
    toString(fd_plasma),
    toString(fd_laser),
    toString( fd_blood_veggy ),
    toString( fd_blood_insect ),
    toString( fd_blood_invertebrate ),
    toString( fd_gibs_insect ),
    toString( fd_gibs_invertebrate )
};
const std::string monster_names[num_monsters] =
{
    toString(mon_null),

    toString(mon_squirrel),
    toString(mon_rabbit),
    toString(mon_deer),
    toString(mon_moose),
    toString(mon_wolf),
    toString(mon_coyote),
    toString(mon_bear),
    toString(mon_cougar),
    toString(mon_crow),

    toString(mon_dog),
    toString(mon_cat),

    toString(mon_ant_larva),
    toString(mon_ant),
    toString(mon_ant_soldier),
    toString(mon_ant_queen),
    toString(mon_ant_fungus),

    toString(mon_fly),
    toString(mon_bee),
    toString(mon_wasp),

    toString(mon_graboid),
    toString(mon_worm),
    toString(mon_halfworm),

    toString(mon_sludge_crawler),

    toString(mon_zombie),
    toString(mon_zombie_cop),
    toString(mon_zombie_shrieker),
    toString(mon_zombie_spitter),
    toString(mon_zombie_electric),
    toString(mon_zombie_smoker),
    toString(mon_zombie_swimmer),
    toString(mon_zombie_dog),
    toString(mon_zombie_brute),
    toString(mon_zombie_hulk),
    toString(mon_zombie_fungus),
    toString(mon_boomer),
    toString(mon_boomer_fungus),
    toString(mon_skeleton),
    toString(mon_zombie_necro),
    toString(mon_zombie_scientist),
    toString(mon_zombie_soldier),
    toString(mon_zombie_grabber),
    toString(mon_zombie_master),
    toString(mon_beekeeper),
    toString(mon_zombie_child),
    toString(mon_zombie_fireman),
    toString(mon_zombie_survivor),

    toString(mon_jabberwock),

    toString(mon_triffid),
    toString(mon_triffid_young),
    toString(mon_triffid_queen),
    toString(mon_creeper_hub),
    toString(mon_creeper_vine),
    toString(mon_biollante),
    toString(mon_vinebeast),
    toString(mon_triffid_heart),

    toString(mon_fungaloid),
    toString(mon_fungaloid_dormant),
    toString(mon_fungaloid_young),
    toString(mon_spore),
    toString(mon_fungaloid_queen),
    toString(mon_fungal_wall),

    toString(mon_blob),
    toString(mon_blob_small),

    toString(mon_chud),
    toString(mon_one_eye),
    toString(mon_crawler),

    toString(mon_sewer_fish),
    toString(mon_sewer_snake),
    toString(mon_sewer_rat),
    toString(mon_rat_king),

    toString(mon_mosquito),
    toString(mon_dragonfly),
    toString(mon_centipede),
    toString(mon_frog),
    toString(mon_slug),
    toString(mon_dermatik_larva),
    toString(mon_dermatik),

    toString(mon_spider_wolf),
    toString(mon_spider_web),
    toString(mon_spider_jumping),
    toString(mon_spider_trapdoor),
    toString(mon_spider_widow),

    toString(mon_dark_wyrm),
    toString(mon_amigara_horror),
    toString(mon_dog_thing),
    toString(mon_headless_dog_thing),
    toString(mon_thing),

    toString(mon_human_snail),
    toString(mon_twisted_body),
    toString(mon_vortex),

    toString(mon_flying_polyp),
    toString(mon_hunting_horror),
    toString(mon_mi_go),
    toString(mon_yugg),
    toString(mon_gelatin),
    toString(mon_flaming_eye),
    toString(mon_kreck),
    toString(mon_gracke),
    toString(mon_blank),
    toString(mon_gozu),
    toString(mon_shadow),
    toString(mon_breather_hub),
    toString(mon_breather),
    toString(mon_shadow_snake),

    toString(mon_dementia),
    toString(mon_homunculus),
    toString(mon_blood_sacrifice),
    toString(mon_flesh_angel),

    toString(mon_eyebot),
    toString(mon_manhack),
    toString(mon_skitterbot),
    toString(mon_secubot),
    toString(mon_hazmatbot),
    toString(mon_copbot),
    toString(mon_molebot),
    toString(mon_tripod),
    toString(mon_chickenbot),
    toString(mon_tankbot),
    toString(mon_turret),
    toString(mon_exploder),

    toString(mon_hallu_mom),

    toString(mon_generator),
// post 0.8
    toString(mon_turkey),
    toString(mon_raccoon),
    toString(mon_opossum),
    toString(mon_rattlesnake),
    toString(mon_giant_crayfish),
// 0.8 -> 0.9
    toString(mon_black_rat),
    toString(mon_mosquito_giant),
    toString(mon_dragonfly_giant),
    toString(mon_centipede_giant),
    toString(mon_frog_giant),
    toString(mon_slug_giant),
    toString(mon_spider_jumping_giant),
    toString(mon_spider_trapdoor_giant),
    toString(mon_spider_web_giant),
    toString(mon_spider_widow_giant),
    toString(mon_spider_wolf_giant),
    toString(mon_bat),
    toString(mon_beaver),
    toString(mon_bobcat),
    toString(mon_chicken),
    toString(mon_chipmunk),
    toString(mon_cow),
    toString(mon_coyote_wolf),
    toString(mon_deer_mouse),
    toString(mon_fox_gray),
    toString(mon_fox_red),
    toString(mon_groundhog),
    toString(mon_hare),
    toString(mon_horse),
    toString(mon_lemming),
    toString(mon_mink),
    toString(mon_muskrat),
    toString(mon_otter),
    toString(mon_pig),
    toString(mon_sheep),
    toString(mon_shrew),
    toString(mon_squirrel_red),
    toString(mon_weasel)

};
const std::string multitile_keys[] =
{
    toString(center),
    toString(corner),
    toString(edge),
    toString(t_connection),
    toString(end_piece),
    toString(unconnected),
    toString(open),
    toString(broken)
};

#endif // TILE_ID_DATA
