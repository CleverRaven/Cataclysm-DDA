#pragma once
#ifndef MONGROUP_H
#define MONGROUP_H

#include <vector>
#include <map>
#include <set>
#include <string>
#include "enums.h"
#include "string_id.h"
#include "calendar.h"
#include "monster.h"

// from overmap.h
class overmap;
class JsonObject;
class JsonIn;
class JsonOut;
struct MonsterGroup;
using mongroup_id = string_id<MonsterGroup>;

struct mtype;
using mtype_id = string_id<mtype>;

struct MonsterGroupEntry;
typedef std::vector<MonsterGroupEntry> FreqDef;
typedef FreqDef::iterator FreqDef_iter;

namespace io
{
struct object_archive_tag;
}

struct MonsterGroupEntry {
    mtype_id name;
    int frequency;
    int cost_multiplier;
    int pack_minimum;
    int pack_maximum;
    std::vector<std::string> conditions;
    time_duration starts;
    time_duration ends;
    bool lasts_forever() const {
        return ( ends <= 0 );
    }

    MonsterGroupEntry( const mtype_id &id, int new_freq, int new_cost,
                       int new_pack_min, int new_pack_max, const time_duration &new_starts, const time_duration &new_ends )
        : name( id )
        , frequency( new_freq )
        , cost_multiplier( new_cost )
        , pack_minimum( new_pack_min )
        , pack_maximum( new_pack_max )
        , starts( new_starts )
        , ends( new_ends ) {
    }
};

struct MonsterGroupResult {
    mtype_id name;
    int pack_size;

    MonsterGroupResult() : name( mtype_id::NULL_ID() ), pack_size( 0 ) {
    }

    MonsterGroupResult( const mtype_id &id, int new_pack_size )
        : name( id ), pack_size( new_pack_size ) {
    }
};

struct MonsterGroup {
    mongroup_id name;
    mtype_id defaultMonster;
    FreqDef  monsters;
    bool IsMonsterInGroup( const mtype_id &id ) const;
    // replaces this group after a period of
    // time when exploring an unexplored portion of the map
    bool replace_monster_group;
    mongroup_id new_monster_group;
    time_duration monster_group_time = 0;
    bool is_safe; /// Used for @ref mongroup::is_safe()
    int freq_total; // Default 1000 unless specified - max number to roll for spawns
};

struct mongroup {
    mongroup_id type;
    // Note: position is not saved as such in the json
    // Instead, a vector of positions is saved for
    tripoint pos = tripoint_zero;
    unsigned int radius = 1;
    unsigned int population = 1;
    tripoint target = tripoint_zero; // location the horde is interested in.
    int interest = 0; //interest to target in percents
    bool dying = false;
    bool horde = false;
    /** This property will be ignored if the vector is empty.
     *  Otherwise it will keep track of the individual monsters that
     *  are contained in this horde, and the population property will
     *  be ignored instead.
     */
    std::vector<monster> monsters;

    /** There are two types of hordes: "city", who try to stick around cities
     *  and return to them whenever possible.
     *  And "roam", who roam around the map randomly, not taking care to return
     *  anywhere.
     */
    std::string horde_behaviour = "";
    bool diffuse = false;   // group size ind. of dist. from center and radius invariant
    mongroup( const mongroup_id &ptype, int pposx, int pposy, int pposz,
              unsigned int prad, unsigned int ppop )
        : type( ptype )
        , pos( pposx, pposy, pposz )
        , radius( prad )
        , population( ppop )
        , target()
        , interest( 0 )
        , dying( false )
        , horde( false )
        , diffuse( false ) {
    }
    mongroup( std::string ptype, tripoint ppos, unsigned int prad, unsigned int ppop,
              tripoint ptarget, int pint, bool pdie, bool phorde, bool pdiff ) :
        type( ptype ), pos( ppos ), radius( prad ), population( ppop ), target( ptarget ),
        interest( pint ), dying( pdie ), horde( phorde ), diffuse( pdiff ) { }
    mongroup() { }
    bool is_safe() const;
    bool empty() const;
    void clear();
    void set_target( int x, int y ) {
        target.x = x;
        target.y = y;
    }
    void wander( overmap & );
    void inc_interest( int inc ) {
        interest += inc;
        if( interest > 100 ) {
            interest = 100;
        }
    }
    void dec_interest( int dec ) {
        interest -= dec;
        if( interest < 15 ) {
            interest = 15;
        }
    }
    void set_interest( int set ) {
        if( set < 15 ) {
            set = 15;
        }
        if( set > 100 ) {
            set = 100;
        }
        interest = set;
    }

    template<typename Archive>
    void io( Archive & );
    using archive_type_tag = io::object_archive_tag;

    void deserialize( JsonIn &jsin );
    void deserialize_legacy( JsonIn &jsin );
    void serialize( JsonOut &jsout ) const;
};

class MonsterGroupManager
{
    public:
        static void LoadMonsterGroup( JsonObject &jo );
        static void LoadMonsterBlacklist( JsonObject &jo );
        static void LoadMonsterWhitelist( JsonObject &jo );
        static void FinalizeMonsterGroups();
        static MonsterGroupResult GetResultFromGroup( const mongroup_id &group, int *quantity = 0 );
        static bool IsMonsterInGroup( const mongroup_id &group, const mtype_id &id );
        static bool isValidMonsterGroup( const mongroup_id &group );
        static const mongroup_id &Monster2Group( const mtype_id &id );
        static std::vector<mtype_id> GetMonstersFromGroup( const mongroup_id &group );
        static const MonsterGroup &GetMonsterGroup( const mongroup_id &group );
        static const MonsterGroup &GetUpgradedMonsterGroup( const mongroup_id &group );
        /**
         * Gets a random monster, weighted by frequency.
         * Ignores cost multiplier.
         */
        static const mtype_id &GetRandomMonsterFromGroup( const mongroup_id &group );

        static void check_group_definitions();

        static void ClearMonsterGroups();

        static bool monster_is_blacklisted( const mtype_id &m );

    private:
        static std::map<mongroup_id, MonsterGroup> monsterGroupMap;
        typedef std::set<std::string> t_string_set;
        static t_string_set monster_blacklist;
        static t_string_set monster_whitelist;
        static t_string_set monster_categories_blacklist;
        static t_string_set monster_categories_whitelist;
};

#endif
