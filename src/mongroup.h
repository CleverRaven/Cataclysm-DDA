#ifndef MONGROUP_H
#define MONGROUP_H

#include "mtype.h"
#include <vector>
#include <map>
#include <set>
#include <string>
#include "json.h"

struct MonsterGroupEntry;
typedef std::vector<MonsterGroupEntry> FreqDef;
typedef FreqDef::iterator FreqDef_iter;

struct MonsterGroupEntry {
    std::string name;
    int frequency;
    int cost_multiplier;
    int pack_minimum;
    int pack_maximum;
    std::vector<std::string> conditions;
    int starts;
    int ends;
    bool lasts_forever()
    {
        return (ends <= 0);
    }

    MonsterGroupEntry(std::string new_name, int new_freq, int new_cost,
                      int new_pack_max, int new_pack_min, int new_starts,
                      int new_ends)
    {
        name = new_name;
        frequency = new_freq;
        cost_multiplier = new_cost;
        pack_minimum = new_pack_min;
        pack_maximum = new_pack_max;
        starts = new_starts;
        ends = new_ends;
    }
};

struct MonsterGroupResult {
    std::string name;
    int pack_size;

    MonsterGroupResult()
    {
        name = "mon_null";
        pack_size = 0;
    }

    MonsterGroupResult(std::string new_name, int new_pack_size)
    {
        name = new_name;
        pack_size = new_pack_size;
    }
};

struct MonsterGroup {
    std::string name;
    std::string defaultMonster;
    FreqDef  monsters;
    bool IsMonsterInGroup(const std::string &mtypeid) const;
    // replaces this group after a period of
    // time when exploring an unexplored portion of the map
    bool replace_monster_group;
    std::string new_monster_group;
    int monster_group_time;  //time in days

};

struct mongroup {
    std::string type;
    int posx, posy, posz;
    unsigned int radius;
    unsigned int population;
    int tx, ty; //horde target
    int interest; //interest to target in percents
    bool dying;
    bool horde;
    bool diffuse;   // group size ind. of dist. from center and radius invariant
    mongroup( std::string ptype, int pposx, int pposy, int pposz,
              unsigned int prad, unsigned int ppop )
    {
        type = ptype;
        posx = pposx;
        posy = pposy;
        posz = pposz;
        radius = prad;
        population = ppop;
        tx = 0;
        ty = 0;
        interest = 0;
        dying = false;
        diffuse = false;
        horde = false;
    }
    bool is_safe()
    {
        return (type == "GROUP_NULL" ||
                type == "GROUP_SAFE" );
    };
    void set_target(int x, int y)
    {
        tx = x;
        ty = y;
    }
    void wander();
    void inc_interest(int inc)
    {
        interest += inc;
        if (interest > 100) {
            interest = 100;
        }
    }
    void dec_interest(int dec)
    {
        interest -= dec;
        if (interest < 15) {
            interest = 15;
        }
    }
    void set_interest(int set)
    {
        if (set < 15) {
            set = 15;
        }
        if (set > 100) {
            set = 100;
        }
        interest = set;
    }
};

class MonsterGroupManager
{
    public:
        static void LoadMonsterGroup(JsonObject &jo);
        static void LoadMonsterBlacklist(JsonObject &jo);
        static void LoadMonsterWhitelist(JsonObject &jo);
        static void FinalizeMonsterGroups();
        static MonsterGroupResult GetResultFromGroup(std::string,
                int *quantity = 0, int turn = -1);
        static bool IsMonsterInGroup(std::string, std::string);
        static std::string Monster2Group(std::string);
        static std::vector<std::string> GetMonstersFromGroup(std::string);
        static MonsterGroup &GetMonsterGroup(std::string group);
        static bool isValidMonsterGroup(std::string group);

        static void check_group_definitions();

        static void ClearMonsterGroups();

        static bool monster_is_blacklisted( const mtype *m );

    private:
        static std::map<std::string, MonsterGroup> monsterGroupMap;
        typedef std::set<std::string> t_string_set;
        static t_string_set monster_blacklist;
        static t_string_set monster_whitelist;
        static t_string_set monster_categories_blacklist;
        static t_string_set monster_categories_whitelist;
};

#endif
