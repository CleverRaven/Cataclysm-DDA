#include "game.h"
#include <fstream>
#include <vector>
#include "setvector.h"
#include "catajson.h"
#include "options.h"

// Default start time, this is the only place it's still used.
#define STARTING_MINUTES 480

// hack for MingW: prevent undefined references to `libintl_printf'
#if defined _WIN32 || defined __CYGWIN__
 #undef printf
#endif

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

std::map<std::string, MonsterGroup> MonsterGroupManager::monsterGroupMap;

void game::init_mongroups() throw (std::string)
{
   try
   {
       MonsterGroupManager::LoadJSONGroups();
   }
   catch(std::string &error_message)
   {
       throw;
   }
}

mon_id MonsterGroupManager::GetMonsterFromGroup( std::string group, std::vector <mtype*> *mtypes,
                                                 int *quantity, int turn )
{
    int roll = rng(1, 1000);
    MonsterGroup g = monsterGroupMap[group];
    for (FreqDef_iter it = g.monsters.begin(); it != g.monsters.end(); ++it)
    {
        if((turn == -1 || (turn + 900 >= MINUTES(STARTING_MINUTES) + HOURS((*mtypes)[it->first]->difficulty))) &&
           (!OPTIONS["CLASSIC_ZOMBIES"] ||
            (*mtypes)[it->first]->in_category(MC_CLASSIC) ||
            (*mtypes)[it->first]->in_category(MC_WILDLIFE)))
        {   //Not too hard for us (or we dont care)
            if(it->second.first >= roll)
            {
                if( quantity) { *quantity -= it->second.second; }
                return it->first;
            }
            else { roll -= it->second.first; }
        }
    }
    if ((turn + 900 < MINUTES(STARTING_MINUTES) + HOURS((*mtypes)[g.defaultMonster]->difficulty))
        && (!OPTIONS["STATIC_SPAWN"]))
    {
        return mon_null;
    }
    else
    {
        return g.defaultMonster;
    }
}

bool MonsterGroupManager::IsMonsterInGroup(std::string group, mon_id monster)
{
    MonsterGroup g = monsterGroupMap[group];
    for (FreqDef_iter it = g.monsters.begin(); it != g.monsters.end(); ++it)
    {
        if(it->first == monster) return true;
    }
    return false;
}

std::string MonsterGroupManager::Monster2Group(mon_id monster)
{
    for (std::map<std::string, MonsterGroup>::const_iterator it = monsterGroupMap.begin(); it != monsterGroupMap.end(); ++it)
    {
        if(IsMonsterInGroup(it->first, monster ))
        {
            return it->first;
        }
    }
    return "GROUP_NULL";
}

std::vector<mon_id> MonsterGroupManager::GetMonstersFromGroup(std::string group)
{
    MonsterGroup g = GetMonsterGroup(group);

    std::vector<mon_id> monsters;

    monsters.push_back(g.defaultMonster);

    for (FreqDef_iter it = g.monsters.begin(); it != g.monsters.end(); ++it)
    {
        monsters.push_back(it->first);
    }
    return monsters;
}

MonsterGroup MonsterGroupManager::GetMonsterGroup(std::string group)
{
    std::map<std::string, MonsterGroup>::iterator it = monsterGroupMap.find(group);
    if(it == monsterGroupMap.end())
    {
        debugmsg("Unable to get the group '%s'", group.c_str());
        return MonsterGroup();
    }
    else
    {
        return it->second;
    }
}

//json loading
const char *monGroupFilePath = "data/raw/monstergroups.json";
std::map<std::string, mon_id> monStr2monId;
void init_translation();
std::string GetString(std::string, picojson::object *);
int GetInt(std::string, picojson::object *);

MonsterGroup GetMGroupFromJSON(picojson::object *jsonobj)
{
    MonsterGroup g;
    picojson::object jsonmonster;
    picojson::value jsonval;
    std::vector<picojson::value> jsonarray;
    g.name = GetString("name", jsonobj);
    g.defaultMonster = monStr2monId[GetString("default", jsonobj)];

    if(jsonobj->find("monsters")->second.is<picojson::array>())
        jsonarray = jsonobj->find("monsters")->second.get<picojson::array>();
    else
    {
        printf("Fatal error, cannot get monster group array in %s", monGroupFilePath);
        exit(1);
    }

    for (picojson::array::const_iterator it_mons = jsonarray.begin(); it_mons != jsonarray.end(); ++it_mons)
    {
        jsonmonster = it_mons->get<picojson::object>();
// todo: Bannination
        g.monsters[monStr2monId[GetString("monster",&jsonmonster)]] =
            std::pair<int,int>(GetInt("freq",&jsonmonster), GetInt("multiplier",&jsonmonster));
    }

    return g;
}

void MonsterGroupManager::LoadJSONGroups() throw (std::string)
{
    //open the file
    std::ifstream file;
    file.open(monGroupFilePath);
    if(!file.good())
    {
        throw (std::string)"Unable to load file " + monGroupFilePath;
    }

    //load the data
    picojson::value groupsRaw;
    file >> groupsRaw;

    /*std::string error = picojson::get_last_error();
    if(! error.empty())
    {
        printf("'%s' : %s", monGroupFilePath, error.c_str());
        return;
    }*/

    //check the data
    if (! groupsRaw.is<picojson::array>()) {
        throw (std::string)"The monster group file " + monGroupFilePath +
              (std::string)" does not contain the expected JSON data";
    }

    init_translation();
    picojson::object jsonobj;
    MonsterGroup g;

    const picojson::array& groups = groupsRaw.get<picojson::array>();
    for (picojson::array::const_iterator it_groups = groups.begin(); it_groups != groups.end(); ++it_groups)
    {
        jsonobj = it_groups->get<picojson::object>();
        g = GetMGroupFromJSON(&jsonobj);
        monsterGroupMap[g.name] = g;
    }
    if(!json_good())
    {
        throw (std::string)"There was an error reading " + monGroupFilePath;
    }
}


std::string GetString(std::string key, picojson::object *obj)
{
    if(obj->find(key)->second.is<std::string>())
        return obj->find(key)->second.get<std::string>();
    else
    {
        printf("Cannot get string '%s' in '%s'",key.c_str(), monGroupFilePath);
    }
    return "";
}

int GetInt(std::string key, picojson::object *obj)
{
    if(obj->find(key)->second.is<double>())
        return obj->find(key)->second.get<double>();
    else
    {
        printf("Cannot get number '%s' in '%s'",key.c_str(), monGroupFilePath);
        return 0;
    }
}

void init_translation()
{
    monStr2monId["mon_null"] = mon_null;
    monStr2monId["mon_squirrel"] = mon_squirrel; monStr2monId["mon_rabbit"] = mon_rabbit; monStr2monId["mon_deer"] = mon_deer; monStr2monId["mon_moose"] = mon_moose; monStr2monId["mon_wolf"] = mon_wolf; monStr2monId["mon_coyote"] = mon_coyote; monStr2monId["mon_bear"] = mon_bear; monStr2monId["mon_cougar"] = mon_cougar; monStr2monId["mon_crow"] = mon_crow;
    monStr2monId["mon_dog"] = mon_dog; monStr2monId["mon_cat"] = mon_cat;
    monStr2monId["mon_ant_larva"] = mon_ant_larva; monStr2monId["mon_ant"] = mon_ant; monStr2monId["mon_ant_soldier"] = mon_ant_soldier; monStr2monId["mon_ant_queen"] = mon_ant_queen; monStr2monId["mon_ant_fungus"] = mon_ant_fungus;
    monStr2monId["mon_fly"] = mon_fly; monStr2monId["mon_bee"] = mon_bee; monStr2monId["mon_wasp"] = mon_wasp;
    monStr2monId["mon_graboid"] = mon_graboid; monStr2monId["mon_worm"] = mon_worm; monStr2monId["mon_halfworm"] = mon_halfworm;
    monStr2monId["mon_zombie"] = mon_zombie; monStr2monId["mon_zombie_cop"] = mon_zombie_cop; monStr2monId["mon_zombie_shrieker"] = mon_zombie_shrieker; monStr2monId["mon_zombie_spitter"] = mon_zombie_spitter; monStr2monId["mon_zombie_electric"] = mon_zombie_electric;
    monStr2monId["mon_zombie_smoker"] = mon_zombie_smoker;
    monStr2monId["mon_zombie_fast"] = mon_zombie_fast; monStr2monId["mon_zombie_brute"] = mon_zombie_brute; monStr2monId["mon_zombie_hulk"] = mon_zombie_hulk; monStr2monId["mon_zombie_fungus"] = mon_zombie_fungus;
    monStr2monId["mon_boomer"] = mon_boomer; monStr2monId["mon_boomer_fungus"] = mon_boomer_fungus; monStr2monId["mon_skeleton"] = mon_skeleton; monStr2monId["mon_zombie_necro"] = mon_zombie_necro;
    monStr2monId["mon_zombie_scientist"] = mon_zombie_scientist; monStr2monId["mon_zombie_soldier"] = mon_zombie_soldier; monStr2monId["mon_zombie_grabber"] = mon_zombie_grabber;
    monStr2monId["mon_zombie_master"] = mon_zombie_master;  monStr2monId["mon_beekeeper"] = mon_beekeeper; monStr2monId["mon_zombie_child"] = mon_zombie_child;
    monStr2monId["mon_triffid"] = mon_triffid; monStr2monId["mon_triffid_young"] = mon_triffid_young; monStr2monId["mon_triffid_queen"] = mon_triffid_queen; monStr2monId["mon_creeper_hub"] = mon_creeper_hub;
    monStr2monId["mon_creeper_vine"] = mon_creeper_vine; monStr2monId["mon_biollante"] = mon_biollante; monStr2monId["mon_vinebeast"] = mon_vinebeast; monStr2monId["mon_triffid_heart"] = mon_triffid_heart;
    monStr2monId["mon_fungaloid"] = mon_fungaloid; monStr2monId["mon_fungaloid_dormant"] = mon_fungaloid_dormant; monStr2monId["mon_fungaloid_young"] = mon_fungaloid_young; monStr2monId["mon_spore"] = mon_spore;
    monStr2monId["mon_fungaloid_queen"] = mon_fungaloid_queen; monStr2monId["mon_fungal_wall"] = mon_fungal_wall;
    monStr2monId["mon_blob"] = mon_blob; monStr2monId["mon_blob_small"] = mon_blob_small;
    monStr2monId["mon_chud"] = mon_chud; monStr2monId["mon_one_eye"] = mon_one_eye; monStr2monId["mon_crawler"] = mon_crawler;
    monStr2monId["mon_sewer_fish"] = mon_sewer_fish; monStr2monId["mon_sewer_snake"] = mon_sewer_snake; monStr2monId["mon_sewer_rat"] = mon_sewer_rat; monStr2monId["mon_rat_king"] = mon_rat_king;
    monStr2monId["mon_mosquito"] = mon_mosquito; monStr2monId["mon_dragonfly"] = mon_dragonfly; monStr2monId["mon_centipede"] = mon_centipede; monStr2monId["mon_frog"] = mon_frog; monStr2monId["mon_slug"] = mon_slug;
    monStr2monId["mon_dermatik_larva"] = mon_dermatik_larva; monStr2monId["mon_dermatik"] = mon_dermatik;
    monStr2monId["mon_jabberwock"] = mon_jabberwock;
    monStr2monId["mon_spider_wolf"] = mon_spider_wolf; monStr2monId["mon_spider_web"] = mon_spider_web; monStr2monId["mon_spider_jumping"] = mon_spider_jumping; monStr2monId["mon_spider_trapdoor"] = mon_spider_trapdoor;
    monStr2monId["mon_spider_widow"] = mon_spider_widow;
    monStr2monId["mon_dark_wyrm"] = mon_dark_wyrm; monStr2monId["mon_amigara_horror"] = mon_amigara_horror; monStr2monId["mon_dog_thing"] = mon_dog_thing; monStr2monId["mon_headless_dog_thing"] = mon_headless_dog_thing;
    monStr2monId["mon_thing"] = mon_thing;
    monStr2monId["mon_human_snail"] = mon_human_snail; monStr2monId["mon_twisted_body"] = mon_twisted_body; monStr2monId["mon_vortex"] = mon_vortex;
    monStr2monId["mon_flying_polyp"] = mon_flying_polyp; monStr2monId["mon_hunting_horror"] = mon_hunting_horror; monStr2monId["mon_mi_go"] = mon_mi_go; monStr2monId["mon_yugg"] = mon_yugg; monStr2monId["mon_gelatin"] = mon_gelatin;
    monStr2monId["mon_flaming_eye"] = mon_flaming_eye; monStr2monId["mon_kreck"] = mon_kreck; monStr2monId["mon_gracke"] = mon_gracke; monStr2monId["mon_blank"] = mon_blank; monStr2monId["mon_gozu"] = mon_gozu; monStr2monId["mon_shadow"] = mon_shadow; monStr2monId["mon_breather_hub"] = mon_breather_hub;
    monStr2monId["mon_breather"] = mon_breather; monStr2monId["mon_shadow_snake"] = mon_shadow_snake;
    monStr2monId["mon_eyebot"] = mon_eyebot; monStr2monId["mon_manhack"] = mon_manhack; monStr2monId["mon_skitterbot"] = mon_skitterbot; monStr2monId["mon_secubot"] = mon_secubot; monStr2monId["mon_copbot"] = mon_copbot; monStr2monId["mon_molebot"] = mon_molebot;
    monStr2monId["mon_tripod"] = mon_tripod; monStr2monId["mon_chickenbot"] = mon_chickenbot; monStr2monId["mon_tankbot"] = mon_tankbot; monStr2monId["mon_turret"] = mon_turret; monStr2monId["mon_exploder"] = mon_exploder;
    monStr2monId["mon_hallu_zom"] = mon_hallu_zom; monStr2monId["mon_hallu_bee"] = mon_hallu_bee; monStr2monId["mon_hallu_ant"] = mon_hallu_ant; monStr2monId["mon_hallu_mom"] = mon_hallu_mom;
    monStr2monId["mon_generator"] = mon_generator;
}

