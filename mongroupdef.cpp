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
//void init_translation();

void game::init_mongroups() throw (std::string)
{
   try
   {
       //init_translation();
       //MonsterGroupManager::LoadJSONGroups();
   }
   catch(std::string &error_message)
   {
       throw;
   }
}

std::string MonsterGroupManager::GetMonsterFromGroup( std::string group, std::vector <mtype*> *mtypes,
                                                 int *quantity, int turn )
{
    int roll = rng(1, 1000);
    MonsterGroup g = monsterGroupMap[group];
    for (FreqDef_iter it = g.monsters.begin(); it != g.monsters.end(); ++it)
    {
        mtype *mon = monster_factory::factory().get_mon(it->first);
        if((turn == -1 || (turn + 900 >= MINUTES(STARTING_MINUTES) + HOURS(mon->difficulty))) &&
           (!OPTIONS["CLASSIC_ZOMBIES"] ||
            mon->in_category(MC_CLASSIC) ||
            mon->in_category(MC_WILDLIFE)))
        {   //Not too hard for us (or we dont care)
            if(it->second.first >= roll)
            {
                if( quantity) { *quantity -= it->second.second; }
                return it->first;
            }
            else { roll -= it->second.first; }
        }
    }
    if ((turn + 900 < MINUTES(STARTING_MINUTES) + HOURS(monster_factory::factory().get_mon(g.defaultMonster)->difficulty))
        && (!OPTIONS["STATIC_SPAWN"]))
    {
        return "mon_null";
    }
    else
    {
        return g.defaultMonster;
    }
}

bool MonsterGroupManager::IsMonsterInGroup(std::string group, std::string monster)
{
    MonsterGroup g = monsterGroupMap[group];
    for (FreqDef_iter it = g.monsters.begin(); it != g.monsters.end(); ++it)
    {
        if(it->first == monster) return true;
    }
    return false;
}

std::string MonsterGroupManager::Monster2Group(std::string monster)
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

std::vector<std::string> MonsterGroupManager::GetMonstersFromGroup(std::string group)
{
    MonsterGroup g = GetMonsterGroup(group);

    std::vector<std::string> monsters;

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
void MonsterGroupManager::load_monster_group(JsonObject &jo)
{
    JsonArray jo_array;

    MonsterGroup g;

    // required
    std::string name = jo.get_string("name");
    std::string mon_default = jo.get_string("default");

    g.name = name;
    g.defaultMonster = mon_default;

    jo_array = jo.get_array("monsters");
    if (jo_array.size() > 0)
    {
        while (jo_array.has_more())
        {
            JsonArray monster_def = jo_array.next_array();
            std::string mon_name = monster_def.get_string(0);
            int mon_frequency = monster_def.get_int(1);
            int mon_multiplier = monster_def.get_int(2);

            g.monsters[mon_name] = std::pair<int,int>(mon_frequency, mon_multiplier);
        }
    }

    monsterGroupMap[g.name] = g;
}
