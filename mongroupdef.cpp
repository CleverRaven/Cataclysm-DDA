#include "game.h"
#include "setvector.h"

void game::init_moncats()
{
 setvector(
   moncats[mcat_forest],
	mon_squirrel, mon_rabbit, mon_deer, mon_wolf, mon_bear, mon_cougar, mon_spider_wolf,
	mon_spider_jumping, mon_dog, mon_crow, NULL);
 setvector(
   moncats[mcat_ant],
	mon_ant_larva, mon_ant, mon_ant_soldier, mon_ant_queen, NULL);
 setvector(
   moncats[mcat_bee],
	mon_bee, NULL);
 setvector(
   moncats[mcat_worm],
	mon_graboid, mon_worm, mon_halfworm, NULL);
 setvector(
   moncats[mcat_zombie],
   // Regular zombie listed twice to get around the max frequency of 255
     mon_zombie, mon_zombie, mon_zombie_cop, mon_zombie_shrieker,
     mon_zombie_spitter, mon_zombie_fast,
  	  mon_zombie_electric, mon_zombie_brute, mon_zombie_hulk,
	    mon_zombie_necro, mon_boomer, mon_skeleton, mon_zombie_grabber,
     mon_zombie_master, mon_crow, NULL);
 setvector(
   moncats[mcat_triffid],
	mon_triffid, mon_triffid_young, mon_vinebeast, mon_triffid_queen, NULL);
 setvector(
   moncats[mcat_fungi],
	mon_fungaloid, mon_fungaloid_dormant, mon_ant_fungus, mon_zombie_fungus,
	mon_boomer_fungus, mon_spore, mon_fungaloid_queen, mon_fungal_wall,
	NULL);
 setvector(
   moncats[mcat_goo],
	mon_blob, NULL);
 setvector(
   moncats[mcat_chud],
	mon_chud, mon_one_eye, mon_crawler, NULL);
 setvector(
   moncats[mcat_sewer],
	mon_sewer_fish, mon_sewer_snake, mon_sewer_rat, NULL);
 setvector(
   moncats[mcat_swamp],
	mon_mosquito, mon_dragonfly, mon_centipede, mon_frog, mon_slug,
	mon_dermatik_larva, mon_dermatik, NULL);
 setvector(
   moncats[mcat_lab],
	mon_zombie_scientist, mon_blob_small, mon_manhack, mon_skitterbot,
	NULL);
 setvector(
   moncats[mcat_nether],
	mon_flying_polyp, mon_hunting_horror, mon_mi_go, mon_yugg, mon_gelatin,
	mon_flaming_eye, mon_kreck, mon_blank, mon_gozu, NULL);
 setvector(
   moncats[mcat_spiral],
	mon_human_snail, mon_twisted_body, mon_vortex, NULL);
 setvector(
   moncats[mcat_vanilla_zombie],
	mon_zombie, NULL);
 setvector(
   moncats[mcat_spider],
	mon_spider_wolf, mon_spider_web, mon_spider_jumping, mon_spider_widow,
	NULL);
 setvector(
   moncats[mcat_robot],
	mon_manhack, mon_skitterbot, mon_secubot, mon_copbot, mon_molebot,
	mon_tripod, mon_chickenbot, mon_tankbot, NULL);
}

MonsterGroup //hello there
forestGroup, antGroup, beeGroup, wormGroup,
zombieGroup, triffidGroup, fungiGroup, gooGroup,
chudGroup, sewerGroup, swampGroup, labGroup,
netherGroup, spiralGroup, vanillaGroup,
spiderGroup, robotGroup;


void game::init_mongroups()
{
    //TODO: Balance the numbers
    forestGroup.defaultMonster = mon_squirrel;
    forestGroup.monsters[mon_rabbit] = 500;//50%
    forestGroup.monsters[mon_deer]   = 250;//25%
    forestGroup.monsters[mon_wolf]   = 200;//20%
    forestGroup.monsters[mon_bear]   = 100;//10%
    forestGroup.monsters[mon_cougar] = 100;//
    forestGroup.monsters[mon_spider_wolf] = 100;
    forestGroup.monsters[mon_spider_jumping] = 100;
    forestGroup.monsters[mon_dog]    = 200;
    forestGroup.monsters[mon_crow]   = 200;

    antGroup.defaultMonster = mon_ant;
    antGroup.monsters[mon_ant_larva]   = 250;
    antGroup.monsters[mon_ant_soldier] = 200;
    antGroup.monsters[mon_ant_queen]   = 50;//5%

    beeGroup.defaultMonster = mon_bee;

    wormGroup.defaultMonster = mon_worm;
    wormGroup.monsters[mon_graboid]  = 300;
    wormGroup.monsters[mon_halfworm] = 300;

    zombieGroup.defaultMonster = mon_zombie;
    zombieGroup.monsters[mon_zombie_cop]    = 200;
    zombieGroup.monsters[mon_zombie_fast]   = 200;
    zombieGroup.monsters[mon_zombie_grabber]= 200;
    zombieGroup.monsters[mon_skeleton]      = 200;

    zombieGroup.monsters[mon_zombie_shrieker]= 100;
    zombieGroup.monsters[mon_zombie_spitter] = 100;
    zombieGroup.monsters[mon_zombie_electric]= 100;
    zombieGroup.monsters[mon_zombie_necro]   = 100;
    zombieGroup.monsters[mon_boomer]         = 100;

    zombieGroup.monsters[mon_zombie_brute]  = 100;
    zombieGroup.monsters[mon_zombie_hulk]   = 50;
    zombieGroup.monsters[mon_zombie_master] = 10;
    zombieGroup.monsters[mon_crow]          = 10;

    triffidGroup.defaultMonster = mon_triffid;
    triffidGroup.monsters[mon_triffid_young] = 100;
    triffidGroup.monsters[mon_vinebeast]     = 100;
    triffidGroup.monsters[mon_triffid_queen] = 100;

    fungiGroup.defaultMonster = mon_fungaloid;
    fungiGroup.monsters[mon_fungaloid_dormant]= 100;
    fungiGroup.monsters[mon_ant_fungus]       = 100;
    fungiGroup.monsters[mon_zombie_fungus]    = 100;
    fungiGroup.monsters[mon_boomer_fungus]    = 100;
    fungiGroup.monsters[mon_spore]            = 100;
    fungiGroup.monsters[mon_fungaloid_queen]  = 100;
    fungiGroup.monsters[mon_fungal_wall]      = 100;

    gooGroup.defaultMonster = mon_blob;

    chudGroup.defaultMonster = mon_chud;
    chudGroup.monsters[mon_one_eye] = 100;
    chudGroup.monsters[mon_crawler] = 100;

    sewerGroup.defaultMonster = mon_sewer_rat;
    sewerGroup.monsters[mon_sewer_fish]  = 100;
    sewerGroup.monsters[mon_sewer_snake] = 100;

    swampGroup.defaultMonster = mon_mosquito;
    swampGroup.monsters[mon_dragonfly] = 100;
    swampGroup.monsters[mon_centipede] = 100;
    swampGroup.monsters[mon_frog]      = 100;
    swampGroup.monsters[mon_slug]      = 100;
    swampGroup.monsters[mon_dermatik_larva] = 100;
    swampGroup.monsters[mon_dermatik]  = 100;

    labGroup.defaultMonster = mon_zombie_scientist;
    labGroup.monsters[mon_blob_small] = 100;
    labGroup.monsters[mon_manhack]    = 100;
    labGroup.monsters[mon_skitterbot] = 100;

    netherGroup.defaultMonster = mon_blank;
    netherGroup.monsters[mon_flying_polyp]  = 100;
    netherGroup.monsters[mon_hunting_horror]= 100;
    netherGroup.monsters[mon_mi_go]         = 100;
    netherGroup.monsters[mon_yugg]          = 100;
    netherGroup.monsters[mon_gelatin]       = 100;
    netherGroup.monsters[mon_flaming_eye]   = 100;
    netherGroup.monsters[mon_kreck]         = 100;
    netherGroup.monsters[mon_gozu]          = 100;

    spiralGroup.defaultMonster = mon_human_snail;
    spiralGroup.monsters[mon_twisted_body] = 100;
    spiralGroup.monsters[mon_vortex]       = 100;

    vanillaGroup.defaultMonster = mon_zombie;

    spiderGroup.defaultMonster = mon_spider_wolf;
    spiderGroup.monsters[mon_spider_web]    = 100;
    spiderGroup.monsters[mon_spider_jumping]= 100;
    spiderGroup.monsters[mon_spider_widow]  = 100;

    robotGroup.defaultMonster = mon_manhack;
    robotGroup.monsters[mon_skitterbot] = 100;
    robotGroup.monsters[mon_secubot]    = 100;
    robotGroup.monsters[mon_copbot]     = 100;
    robotGroup.monsters[mon_molebot]    = 100;
    robotGroup.monsters[mon_tripod]     = 100;
    robotGroup.monsters[mon_chickenbot] = 100;
    robotGroup.monsters[mon_tankbot]    = 100;
}

mon_id GetMonsterFromGroup(MonsterGroup *group)
{
    int roll;// = rng(1, 1000);
    for (FreqDef_iter it = group->monsters.begin(); it != group->monsters.end(); ++it)
    {   //first = mon_id, second = chance%1000
        roll = rng(1, 1000);
        if(it->second >= roll) return it->first;
        else roll -= it->second;
    }
    return group->defaultMonster;
}

bool moncat_is_safe(moncat_id id)
{
 if (id == mcat_null || id == mcat_forest)
  return true;
 return false;
}
