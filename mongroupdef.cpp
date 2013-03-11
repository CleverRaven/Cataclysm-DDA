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
    forestGroup.defaultMonster = mon_squirrel;//freq 50
    forestGroup.monsters[mon_rabbit] = 100;//freq 10
    forestGroup.monsters[mon_deer]   =  30;
    forestGroup.monsters[mon_wolf]   =  40;
    forestGroup.monsters[mon_bear]   =  20;
    forestGroup.monsters[mon_cougar] =  30;//
    forestGroup.monsters[mon_spider_wolf] = 100;
    forestGroup.monsters[mon_spider_jumping] = 100;
    forestGroup.monsters[mon_dog]    =  10;
    forestGroup.monsters[mon_crow]   =  30;
    //mon_dog_thing / tentacle dog ? freq 1
    //mon_cat ? freq 5

    antGroup.defaultMonster = mon_ant;//freq 20
    antGroup.monsters[mon_ant_larva]   =  10;
    antGroup.monsters[mon_ant_soldier] =  20;
    antGroup.monsters[mon_ant_queen]   =   0;//yes freq 0 according to mtypedef.cpp

    beeGroup.defaultMonster = mon_bee;//freq 2
    //mon_beekeeper ?? scarred zombie ? freq=15

    wormGroup.defaultMonster = mon_worm;//freq 30
    wormGroup.monsters[mon_graboid]  =  10;
    wormGroup.monsters[mon_halfworm] =   0;//yes
    //mon_dark_wyrm ? freq 1

    zombieGroup.defaultMonster = mon_zombie;//freq 255
    zombieGroup.monsters[mon_zombie_cop]    = 150;
    zombieGroup.monsters[mon_zombie_fast]   = 500;
    zombieGroup.monsters[mon_zombie_grabber]= 500;
    zombieGroup.monsters[mon_skeleton]      = 250;

    zombieGroup.monsters[mon_zombie_shrieker]= 250;
    zombieGroup.monsters[mon_zombie_spitter] = 250;
    zombieGroup.monsters[mon_zombie_electric]= 250;
    zombieGroup.monsters[mon_zombie_necro]   = 10;
    zombieGroup.monsters[mon_boomer]         = 250;

    zombieGroup.monsters[mon_zombie_brute]  = 100;
    zombieGroup.monsters[mon_zombie_hulk]   = 10;
    zombieGroup.monsters[mon_zombie_master] = 10;
    zombieGroup.monsters[mon_crow]          = 10;
    //mon_zombie_soldier ? freq 3
    //mon_zombie_child ? freq 25
    //mon_zombie_smoker ? freq 25

    triffidGroup.defaultMonster = mon_triffid; //freq 24
    triffidGroup.monsters[mon_triffid_young] = 150;
    triffidGroup.monsters[mon_vinebeast]     = 80;
    triffidGroup.monsters[mon_triffid_queen] = 30;
    //mon_creeper_hub ? freq 0
    //mon_creeper_vine ? freq 0
    //mon_biollante ? freq 0
    //mon_triffid_heart ? freq 0

    fungiGroup.defaultMonster = mon_fungaloid;//freq 12
    fungiGroup.monsters[mon_fungaloid_dormant]=   0;//yes...
    fungiGroup.monsters[mon_ant_fungus]       =   0;//fungal insect ?
    fungiGroup.monsters[mon_zombie_fungus]    = 100;
    fungiGroup.monsters[mon_boomer_fungus]    =  10;
    fungiGroup.monsters[mon_spore]            =   0;//yes...
    fungiGroup.monsters[mon_fungaloid_queen]  = 100;
    fungiGroup.monsters[mon_fungal_wall]      =   0;//yes...
    //mon_fungaloid_young ? freq 6
    //fungal spire? freq 0

    gooGroup.defaultMonster = mon_blob;//freq 10
    gooGroup.monsters[mon_blob_small] = 10;// Added this one because it makes 108% sense

    chudGroup.defaultMonster = mon_chud;//freq 50 dang
    chudGroup.monsters[mon_one_eye] =  50;
    chudGroup.monsters[mon_crawler] =  20;

    sewerGroup.defaultMonster = mon_sewer_rat;//freq 18
    sewerGroup.monsters[mon_sewer_fish]  = 300;
    sewerGroup.monsters[mon_sewer_snake] = 150;
    //mon_rat_king? freq 0

    swampGroup.defaultMonster = mon_mosquito;//freq 22
    swampGroup.monsters[mon_dragonfly] =  60;
    swampGroup.monsters[mon_centipede] =  70;
    swampGroup.monsters[mon_frog]      =  50;
    swampGroup.monsters[mon_slug]      =  40;
    swampGroup.monsters[mon_dermatik_larva] = 0;//yes...
    swampGroup.monsters[mon_dermatik]  = 30;

    labGroup.defaultMonster = mon_zombie_scientist; //freq=20 from mtypedef.cpp
    labGroup.monsters[mon_blob_small] =  10;
    labGroup.monsters[mon_manhack]    = 100;
    labGroup.monsters[mon_skitterbot] = 100;

    netherGroup.defaultMonster = mon_blank;//freq 3
    netherGroup.monsters[mon_flying_polyp]  =  10;
    netherGroup.monsters[mon_hunting_horror]= 100;//freq 10
    netherGroup.monsters[mon_mi_go]         =  50;
    netherGroup.monsters[mon_yugg]          =  30;
    netherGroup.monsters[mon_gelatin]       =  50;
    netherGroup.monsters[mon_flaming_eye]   =  50;
    netherGroup.monsters[mon_kreck]         =  90;
    netherGroup.monsters[mon_gozu]          =  10;
    //mon_gracke ? freq 3
    //mon_shadow ? freq 0
    //mon_breather & mon_breather_hub ? freq 0

    spiralGroup.defaultMonster = mon_human_snail;//freq 20
    spiralGroup.monsters[mon_twisted_body] =  50;
    spiralGroup.monsters[mon_vortex]       =  20;

    vanillaGroup.defaultMonster = mon_zombie;

    spiderGroup.defaultMonster = mon_spider_wolf;//freq 1
    spiderGroup.monsters[mon_spider_web]    =  10;
    spiderGroup.monsters[mon_spider_jumping]=  20;
    spiderGroup.monsters[mon_spider_widow]  =  10;
    //mon_spider_widow ? freq 1

    robotGroup.defaultMonster = mon_manhack;//freq 18
    robotGroup.monsters[mon_skitterbot] = 100;//freq 10
    robotGroup.monsters[mon_secubot]    =  70;
    robotGroup.monsters[mon_copbot]     =   0;//yes
    robotGroup.monsters[mon_molebot]    =  20;
    robotGroup.monsters[mon_tripod]     =  50;
    robotGroup.monsters[mon_chickenbot] =  30;
    robotGroup.monsters[mon_tankbot]    =  10;
    //mon_eyebot ? freq 20


    //unassigned
    //mon_amigara_horror //Amigara horror ? freq 1
    //mon_thing //Thing ? freq 1
    //mon_turret //turret ? freq 0
    //mon_exploder //exploder ? freq 0
    //mon_fly //giant fly ? freq 0
    //mon_wasp //giant wasp ? freq 2

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
