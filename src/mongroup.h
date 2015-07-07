#ifndef MONGROUP_H
#define MONGROUP_H

#include <vector>
#include <map>
#include <set>
#include <string>
#include "enums.h"
#include "json.h"
#include "string_id.h"

struct MonsterGroup;
using mongroup_id = string_id<MonsterGroup>;

struct mtype;

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
    bool lasts_forever() const
    {
        return (ends <= 0);
    }

    MonsterGroupEntry(std::string new_name, int new_freq, int new_cost,
                      int new_pack_max, int new_pack_min, int new_starts,
                      int new_ends)
    : name( new_name )
    , frequency( new_freq )
    , cost_multiplier( new_cost )
    , pack_minimum( new_pack_min )
    , pack_maximum( new_pack_max )
    , starts( new_starts )
    , ends( new_ends )
    {
    }
};

struct MonsterGroupResult {
    std::string name;
    int pack_size;

    MonsterGroupResult()
    : name( "mon_null" )
    , pack_size( 0 )
    {
    }

    MonsterGroupResult(std::string new_name, int new_pack_size)
    : name(new_name )
    , pack_size( new_pack_size )
    {
    }
};

struct MonsterGroup {
    mongroup_id name;
    std::string defaultMonster;
    FreqDef  monsters;
    bool IsMonsterInGroup(const std::string &mtypeid) const;
    // replaces this group after a period of
    // time when exploring an unexplored portion of the map
    bool replace_monster_group;
    mongroup_id new_monster_group;
    int monster_group_time;  //time in days
    bool is_safe; /// Used for @ref mongroup::is_safe()
};

struct mongroup : public JsonSerializer, public JsonDeserializer {
    mongroup_id type;
    tripoint pos;
    unsigned int radius;
    unsigned int population;
    tripoint target; // location the horde is interested in.
    int interest; //interest to target in percents
    bool dying;
    bool horde;
    bool diffuse;   // group size ind. of dist. from center and radius invariant
    mongroup( const mongroup_id& ptype, int pposx, int pposy, int pposz,
              unsigned int prad, unsigned int ppop )
    : type( ptype )
    , pos( pposx, pposy, pposz )
    , radius( prad )
    , population( ppop )
    , target()
    , interest( 0 )
    , dying( false )
    , horde( false )
    , diffuse( false )
    {
    }
    mongroup( std::string ptype, tripoint ppos, unsigned int prad, unsigned int ppop,
              tripoint ptarget, int pint, bool pdie, bool phorde, bool pdiff ) :
    type(ptype), pos(ppos), radius(prad), population(ppop), target(ptarget),
      interest(pint), dying(pdie), horde(phorde), diffuse(pdiff) { }
    mongroup() { }
    bool is_safe() const;
    void set_target(int x, int y)
    {
        target.x = x;
        target.y = y;
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
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin) override;

    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const override;
};

class MonsterGroupManager
{
    public:
        static void LoadMonsterGroup(JsonObject &jo);
        static void LoadMonsterBlacklist(JsonObject &jo);
        static void LoadMonsterWhitelist(JsonObject &jo);
        static void FinalizeMonsterGroups();
        static MonsterGroupResult GetResultFromGroup(const mongroup_id& group,
                int *quantity = 0, int turn = -1);
        static bool IsMonsterInGroup(const mongroup_id& group, const std::string& mtype_id);
        static bool isValidMonsterGroup(const mongroup_id& group);
        static const mongroup_id& Monster2Group(std::string);
        static std::vector<std::string> GetMonstersFromGroup(const mongroup_id& group);
        static const MonsterGroup &GetMonsterGroup(const mongroup_id& group);
        static const MonsterGroup &GetUpgradedMonsterGroup(const mongroup_id& group);

        static void check_group_definitions();

        static void ClearMonsterGroups();

        static bool monster_is_blacklisted( const mtype *m );

    private:
        static std::map<mongroup_id, MonsterGroup> monsterGroupMap;
        typedef std::set<std::string> t_string_set;
        static t_string_set monster_blacklist;
        static t_string_set monster_whitelist;
        static t_string_set monster_categories_blacklist;
        static t_string_set monster_categories_whitelist;
};

#endif
