#include "game.h"
#include "setvector.h"

//Adding a group:
//  1: Declare it in the MonsterGroupDefs enum in mongroup.h
//  2: Define it in here with the macro Group(your group, default monster)
//     and AddMonster(your group, some monster, a frequency on 1000)
//
//  Frequency: If you don't use the whole 1000 points of frequency for each of
//     the monsters, the remaining points will go to the defaultMonster.
//     Ie. a group with 1 monster at frequency will have 50% chance to spawn
//     the default monster.
//     In the same spirit, if you have a total point count of over 1000, the
//     default monster will never get picked, and nor will the others past the
//     monster that makes the point count go over 1000

MonsterGroup MonsterGroupManager::monsterGroupArray[GROUP_COUNT];

//I could do this, or put this \/ giant line in game::game() and have it stick out like a sore thumb :S
void game::init_mongroups() { MonsterGroupManager::init_mongroups(); }
void MonsterGroupManager::init_mongroups()
{
    #define Group(group, defaultmon)     monsterGroupArray[group].defaultMonster = defaultmon;
    #define AddMonster(group, mon, freq) monsterGroupArray[group].monsters[mon]  = freq;

    //TODO: Balance the numbers
    Group(      GROUP_FOREST, mon_squirrel);//old 50 -> 520
    AddMonster( GROUP_FOREST, mon_rabbit,  104);//old 10
    AddMonster( GROUP_FOREST, mon_deer,     31);//old 3
    AddMonster( GROUP_FOREST, mon_wolf,     41);//old 4
    AddMonster( GROUP_FOREST, mon_bear,     21);//old 2
    AddMonster( GROUP_FOREST, mon_cougar,   31);//old 3
    AddMonster( GROUP_FOREST, mon_spider_wolf,   104);//old 10
    AddMonster( GROUP_FOREST, mon_spider_jumping,104);//old 10
    AddMonster( GROUP_FOREST, mon_dog,      10);//old 1
    AddMonster( GROUP_FOREST, mon_crow,     31);//old 3
    //mon_dog_thing / tentacle dog ? freq 1
    //mon_cat ? freq 5

    Group(GROUP_ANT,      mon_ant);              //old 20 ->869
    AddMonster(GROUP_ANT, mon_ant_larva,     43);//old 1
    AddMonster(GROUP_ANT, mon_ant_soldier,   86);//old 2
    AddMonster(GROUP_ANT, mon_ant_queen,      0);//old 0

    Group(GROUP_BEE,  mon_bee);//freq 2 -> 1000
    //mon_beekeeper ?? scarred zombie ? freq15

    Group(GROUP_WORM,      mon_worm);          //freq 30 -> 968
    AddMonster(GROUP_WORM, mon_graboid,    32);//old 1
    AddMonster(GROUP_WORM, mon_halfworm,    0);//old 0
    //mon_dark_wyrm ? freq 1

    Group(GROUP_ZOMBIE,      mon_zombie);            //old 510 -> 666 %1000
    AddMonster(GROUP_ZOMBIE, mon_zombie_cop,     19);//old 15
    AddMonster(GROUP_ZOMBIE, mon_zombie_fast,    65);//old 50
    AddMonster(GROUP_ZOMBIE, mon_zombie_grabber, 65);//old 50
    AddMonster(GROUP_ZOMBIE, mon_skeleton,       32);//old 25

    AddMonster(GROUP_ZOMBIE, mon_zombie_shrieker, 32);//old 25
    AddMonster(GROUP_ZOMBIE, mon_zombie_spitter,  32);//old 25
    AddMonster(GROUP_ZOMBIE, mon_zombie_electric, 32);//old 25
    AddMonster(GROUP_ZOMBIE, mon_zombie_necro,     1);//old 1
    AddMonster(GROUP_ZOMBIE, mon_boomer,          32);//old 25

    AddMonster(GROUP_ZOMBIE, mon_zombie_brute,   13);//old 10
    AddMonster(GROUP_ZOMBIE, mon_zombie_hulk,    1); //old 1
    AddMonster(GROUP_ZOMBIE, mon_zombie_master,  1); //old 1
    AddMonster(GROUP_ZOMBIE, mon_crow,           1); //old 1
    //mon_zombie_soldier ? freq 3
    //mon_zombie_child ? freq 25
    //mon_zombie_smoker ? freq 25

    Group(GROUP_TRIFFID,      mon_triffid); //freq 24 -> 480
    AddMonster(GROUP_TRIFFID, mon_triffid_young,  300);//old 15
    AddMonster(GROUP_TRIFFID, mon_vinebeast,      160);//old 8
    AddMonster(GROUP_TRIFFID, mon_triffid_queen,  60);//old 3
    //mon_creeper_hub ? freq 0
    //mon_creeper_vine ? freq 0
    //mon_biollante ? freq 0
    //mon_triffid_heart ? freq 0

    Group(GROUP_FUNGI,      mon_fungaloid);             //old 12 -> 363
    AddMonster(GROUP_FUNGI, mon_fungaloid_dormant,   0);//old 0
    AddMonster(GROUP_FUNGI, mon_ant_fungus,          0);//old 0
    AddMonster(GROUP_FUNGI, mon_zombie_fungus,     303);//old 10
    AddMonster(GROUP_FUNGI, mon_boomer_fungus,      30);//old 1
    AddMonster(GROUP_FUNGI, mon_spore,               0);//old 0
    AddMonster(GROUP_FUNGI, mon_fungaloid_queen,   303);//old 10
    AddMonster(GROUP_FUNGI, mon_fungal_wall,         0);//old 0
    //mon_fungaloid_young ? freq 6
    //fungal spire? freq 0

    Group(GROUP_GOO,      mon_blob);//freq 10 -> 100% // 80%
    AddMonster(GROUP_GOO, mon_blob_small,  200);// Added this one because it makes 108% sense

    Group(GROUP_CHUD,      mon_chud);         //old 50 -> 877
    AddMonster(GROUP_CHUD, mon_one_eye,   87);//old 5
    AddMonster(GROUP_CHUD, mon_crawler,   35);//old 2

    Group(GROUP_SEWER,      mon_sewer_rat);        //old 18 -> 256
    AddMonster(GROUP_SEWER, mon_sewer_fish,   476);//old 30
    AddMonster(GROUP_SEWER, mon_sewer_snake,  238);//old 15
    //mon_rat_king? freq 0

    Group(GROUP_SWAMP,      mon_mosquito);       //freq 22 -> 468
    AddMonster(GROUP_SWAMP, mon_dragonfly,   127);//old 6
    AddMonster(GROUP_SWAMP, mon_centipede,   149);//old 7
    AddMonster(GROUP_SWAMP, mon_frog,        106);//old 5
    AddMonster(GROUP_SWAMP, mon_slug,        85);//old 4
    AddMonster(GROUP_SWAMP, mon_dermatik_larva,  0);//old 0
    AddMonster(GROUP_SWAMP, mon_dermatik,    64);//old 3

    Group(GROUP_LAB,      mon_zombie_scientist);//old 20 -> 4878
    AddMonster(GROUP_LAB, mon_blob_small,   24);//old 1
    AddMonster(GROUP_LAB, mon_manhack,     244);//old 10
    AddMonster(GROUP_LAB, mon_skitterbot,  244);//old 10

    Group(GROUP_NETHER,      mon_blank);              //old 3 -> 71
    AddMonster(GROUP_NETHER, mon_flying_polyp,    24);//old 1
    AddMonster(GROUP_NETHER, mon_hunting_horror, 238);//old 10
    AddMonster(GROUP_NETHER, mon_mi_go,          119);//old 5
    AddMonster(GROUP_NETHER, mon_yugg,            71);//old 3
    AddMonster(GROUP_NETHER, mon_gelatin,        119);//old 5
    AddMonster(GROUP_NETHER, mon_flaming_eye,    119);//old 5
    AddMonster(GROUP_NETHER, mon_kreck,          214);//old 9
    AddMonster(GROUP_NETHER, mon_gozu,            24);//old 1
    //mon_gracke ? freq 3
    //mon_shadow ? freq 0
    //mon_breather & mon_breather_hub ? freq 0

    Group(GROUP_SPIRAL,      mon_human_snail);       //old 20 -> 740
    AddMonster(GROUP_SPIRAL, mon_twisted_body,  185);//old 5
    AddMonster(GROUP_SPIRAL, mon_vortex,         74);//old 2

    Group(GROUP_VANILLA,  mon_zombie);

    Group(GROUP_SPIDER,     mon_spider_wolf);        //old 1 -> 200
    AddMonster(GROUP_SPIDER,mon_spider_web,     200);//old 1
    AddMonster(GROUP_SPIDER,mon_spider_jumping, 400);//old 2
    AddMonster(GROUP_SPIDER,mon_spider_widow,   200);//old 1
    //mon_spider_widow ? freq 1

    Group(GROUP_ROBOT,      mon_manhack);         //old 18 -> 391
    AddMonster(GROUP_ROBOT, mon_skitterbot,  217);//old 10
    AddMonster(GROUP_ROBOT, mon_secubot,     152);//old 7
    AddMonster(GROUP_ROBOT, mon_copbot,        0);//old 0
    AddMonster(GROUP_ROBOT, mon_molebot,      43);//old 2
    AddMonster(GROUP_ROBOT, mon_tripod,      108);//old 5
    AddMonster(GROUP_ROBOT, mon_chickenbot,   65);//old 3
    AddMonster(GROUP_ROBOT, mon_tankbot,      21);//old 1
    //mon_eyebot ? freq 20


    //unassigned
    //mon_amigara_horror //Amigara horror ? freq 1
    //mon_thing //Thing ? freq 1
    //mon_turret //turret ? freq 0
    //mon_exploder //exploder ? freq 0
    //mon_fly //giant fly ? freq 0
    //mon_wasp //giant wasp ? freq 2

}

mon_id MonsterGroupManager::GetMonsterFromGroup(MonsterGroupType group)
{
    return GetMonsterFromGroup(group, -1, NULL);
}

mon_id MonsterGroupManager::GetMonsterFromGroup(MonsterGroupType group, int turn, std::vector <mtype*> *mtypes)
{
    int roll = rng(1, 1000);
    MonsterGroup g = monsterGroupArray[group];
    for (FreqDef_iter it = g.monsters.begin(); it != g.monsters.end(); ++it)
    {
        if(turn == -1 || (turn + 900 >= MINUTES(STARTING_MINUTES) + (*mtypes)[it->first]->difficulty))
        {   //Not too hard for us (or we dont care)
            if(it->second >= roll) return it->first;
            else roll -= it->second;
        }
    }
    return g.defaultMonster;
}

bool MonsterGroupManager::IsMonsterInGroup(MonsterGroupType group, mon_id monster)
{
    MonsterGroup g = monsterGroupArray[group];
    for (FreqDef_iter it = g.monsters.begin(); it != g.monsters.end(); ++it)
    {
        if(it->first == monster) return true;
    }
    return false;
}

MonsterGroupType MonsterGroupManager::Monster2Group(mon_id monster)
{
    for(int i = 0; i<GROUP_COUNT; i++)
    {
        if(IsMonsterInGroup( (MonsterGroupType)i, monster ))
        {
            return (MonsterGroupType)i;
        }
    }
    return GROUP_NULL;
}

std::vector<mon_id> MonsterGroupManager::GetMonstersFromGroup(MonsterGroupType group)
{
    std::vector<mon_id> monsters;
    MonsterGroup g = monsterGroupArray[group];

    for (FreqDef_iter it = g.monsters.begin(); it != g.monsters.end(); ++it)
    {
        monsters.push_back(it->first);
    }
    return monsters;
}

